#!/usr/bin/env python3
"""Extend Route 206's side scenery and close its exposed rock profiles.

Asset-specific static mesh edit; permissions, props, BDHC and original geometry
are preserved. Each chunk joins the altitude of its existing blocked filler.
"""
import argparse
import hashlib
from pathlib import Path
import math
import struct
from map_scenery import read_land, repack_land, u32, pack_commands

# Inspected SBC bindings: flat rock / rounded rock / additional edge shapes.
PROFILES = {27: (112, 1, (0, 1)),
            28: (64, 2, (2, 3, 5, 6, 9)),
            29: (64, 1, (0, 1)),
            30: (128, 1, (0, 1))}
SOURCE_HASHES = {
    27: "64e9e81c407401d20195d911b9f7f7f7dbe6ad4a676068533b4e4949c35a1dbe",
    28: "d8b68202a41ef3268da7f19519808fd3da68b8c824c2d4f022b207a0a9ebae4d",
    29: "7fd1ecc22911c271477bc733df14a8790afda7d68df2fb84b1ad5369ccffd540",
    30: "c41edc0d7a995d9caacb7325d0e42ce5d156061967d197945d63cd6cdf3048d9",
}


def compact_quad(vertices, scale, state):
    """Emit the same GX attributes with compressed axis-only vertex updates."""
    commands = [(0x40, (1,))]
    previous = None
    attributes = state[0]
    for p, uv, color, normal in vertices:
        coords = [round(c / scale * 4096) for c in p]
        tex = [round(c * 16) for c in uv]
        if not all(-32768 <= c <= 32767 for c in coords + tex):
            raise ValueError('Scenery exceeds GX fixed-point encoding')
        if attributes is None or attributes[0] != color:
            commands.append((0x20, (color,)))
        if attributes is None or attributes[1] != normal:
            commands.append((0x21, (normal,)))
        attributes = color, normal
        commands.append((0x22, ((tex[0] & 65535) | ((tex[1] & 65535) << 16),)))
        unchanged = next((i for i in range(3) if previous and coords[i] == previous[i]), None)
        if all(c % 64 == 0 and -512 <= c // 64 <= 511 for c in coords):
            commands.append((0x24, (sum(((c // 64) & 1023) << (10 * i) for i, c in enumerate(coords)),)))
        elif unchanged is None:
            commands.append((0x23, ((coords[0] & 65535) | ((coords[1] & 65535) << 16), coords[2] & 65535)))
        else:
            axes, op = {0: ((1, 2), 0x27), 1: ((0, 2), 0x26), 2: ((0, 1), 0x25)}[unchanged]
            commands.append((op, ((coords[axes[0]] & 65535) | ((coords[axes[1]] & 65535) << 16),)))
        previous = coords
    commands.append((0x41, ()))
    state[0] = attributes
    return pack_commands(commands)


def compact_display_list(quads, scale):
    state = [None]
    return b"".join(compact_quad(q, scale, state) for q in quads)


def extend(source, member):
    if hashlib.sha256(source).hexdigest() != SOURCE_HASHES[member]:
        raise ValueError(f"Expected the original Route 206 member {member}")
    model, _, mdl, _, shapes = read_land(source)
    filler_y, cliff, profile_shapes = PROFILES[member]
    additions = [[] for _ in shapes]
    edges = set()
    for shape in profile_shapes:
        for polygon in shapes[shape][4]:
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                pa, pb = a[0], b[0]
                if (pa[0] != pb[0] or abs(pa[0]) != 256 or pa[2] == pb[2]
                        ):
                    continue
                key = tuple(sorted((pa, pb)))
                if key in edges:
                    continue
                edges.add(key)
                distance = 224 if pa[0] > 0 else -224
                # Continue each authored boundary edge, preserving the texture
                # gradient from the original polygon in the extrusion direction.
                gradient = (0, 0)
                for v in polygon:
                    if v[0][0] != pa[0] and v[0][1:] == pa[1:]:
                        gradient = tuple((a[1][i] - v[1][i]) / (pa[0] - v[0][0]) for i in range(2))
                        break
                def shifted(v):
                    p, uv, color, normal = v
                    return ((p[0] + distance, p[1], p[2]),
                            tuple(uv[i] + gradient[i] * distance for i in range(2)), color, normal)
                additions[shape].append([b, a, shifted(a), shifted(b)])
                if pa[1] == pb[1] == filler_y:
                    continue
                points = [shifted(b)[0], shifted(a)[0],
                          (pa[0] + distance, filler_y, pa[2]),
                          (pb[0] + distance, filler_y, pb[2])]
                above = (pa[1] + pb[1]) / 2 > filler_y
                if not above:
                    points.reverse()
                normal = (0x1FF if pa[0] > 0 else 0x201)
                if not above:
                    normal = 0x201 if pa[0] > 0 else 0x1FF
                additions[cliff].append([(p, (p[2] * 2, -p[1] * 2),
                                          a[2], normal) for p in points])
    # Recompute from actual vertices: authored bounds round some bridge points
    # inward. Round outward at the bounding-box fixed-point precision.
    box_scale = struct.unpack_from('<i', model, mdl + 56)[0] / 4096
    quantum = box_scale / 4096
    points = [v[0] for shape in shapes for polygon in shape[4] for v in polygon]
    points += [v[0] for extra in additions for polygon in extra for v in polygon]
    low = [math.floor(min(p[i] for p in points) / quantum) * quantum for i in range(3)]
    high = [math.ceil(max(p[i] for p in points) / quantum) * quantum for i in range(3)]
    bounds = low + [high[i] - low[i] for i in range(3)]
    return repack_land(source, additions, bounds, encode_extra=compact_display_list)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('member', type=int, choices=PROFILES)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result, count = extend(args.input.read_bytes(), args.member)
    args.output.write_bytes(result)
    print(f'Route 206 member {args.member}: {count} side quads; model {u32(result, 8)} bytes')


if __name__ == '__main__':
    main()
