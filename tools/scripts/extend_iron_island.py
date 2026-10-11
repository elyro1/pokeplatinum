#!/usr/bin/env python3
"""Complete Iron Island's decorative terrain for the PC widescreen camera.

Read the original Platinum land BIN and extend its existing display lists.
No model converter is needed. Permissions, props, BDHC, materials and original
polygons are preserved; only the model and its bounds/counts change.
This intentionally supports one known static model, not arbitrary NSBMD files.
"""

import argparse
import hashlib
import struct
from collections import Counter
from pathlib import Path


SOURCE_SHA256 = "9c000bcd8f0bbd80d0e377952c5e9cd35a6986f71453fb84c18b1a1e21ddfc2b"
PARAM_COUNTS = {0: 0, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2,
                0x24: 1, 0x25: 1, 0x26: 1, 0x27: 1, 0x28: 1,
                0x40: 1, 0x41: 0}


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def signed(value, bits):
    return (value ^ (1 << (bits - 1))) - (1 << (bits - 1))


def dictionary(data, offset):
    entries = offset + u16(data, offset + 6)
    stride = u16(data, entries)
    names = entries + u16(data, entries + 2)
    return [(data[names + i * 16:names + (i + 1) * 16].split(b"\0")[0].decode(),
             entries + 4 + i * stride) for i in range(data[offset + 1])]


def polygons(display_list, scale):
    """Decode the commands used by the original static Iron Island mesh."""
    offset = 0
    position = [0, 0, 0]
    uv = (0, 0)
    color, normal, mode = 0x7FFF, 0x0007FC00, None
    vertices, result = [], []
    while offset < len(display_list):
        commands = display_list[offset:offset + 4]
        offset += 4
        for command in commands:
            count = PARAM_COUNTS[command]
            args = struct.unpack_from("<" + "I" * count, display_list, offset)
            offset += count * 4
            if command == 0x20:
                color = args[0]
            elif command == 0x21:
                normal = args[0]
            elif command == 0x22:
                uv = (signed(args[0] & 0xFFFF, 16) / 16,
                      signed(args[0] >> 16, 16) / 16)
            elif command == 0x40:
                mode, vertices = args[0], []
            elif command == 0x41:
                mode = None
            elif command == 0x23:
                position = [signed(args[0] & 0xFFFF, 16) / 4096,
                            signed(args[0] >> 16, 16) / 4096,
                            signed(args[1] & 0xFFFF, 16) / 4096]
            elif command == 0x24:
                position = [signed((args[0] >> (i * 10)) & 1023, 10) / 64
                            for i in range(3)]
            elif command in (0x25, 0x26, 0x27):
                for i, dimension in enumerate({0x25: (0, 1), 0x26: (0, 2),
                                               0x27: (1, 2)}[command]):
                    position[dimension] = signed((args[0] >> (i * 16)) & 0xFFFF, 16) / 4096
            elif command == 0x28:
                position = [position[i] + signed((args[0] >> (i * 10)) & 1023, 10) / 4096
                            for i in range(3)]
            if 0x23 <= command <= 0x28:
                vertices.append((tuple(c * scale for c in position), uv, color, normal))
                n = len(vertices)
                if mode == 0 and n % 3 == 0:
                    result.append(vertices[-3:])
                elif mode == 1 and n % 4 == 0:
                    result.append(vertices[-4:])
                elif mode == 2 and n >= 3:
                    result.append(vertices[-3:] if n % 2 else
                                  [vertices[-2], vertices[-3], vertices[-1]])
                elif mode == 3 and n >= 4 and n % 2 == 0:
                    result.append([vertices[-4], vertices[-3], vertices[-1], vertices[-2]])
    return result


def pack_commands(commands):
    result = bytearray()
    for i in range(0, len(commands), 4):
        group = commands[i:i + 4]
        result.extend(bytes(c[0] for c in group).ljust(4, b"\0"))
        for _, params in group:
            result.extend(struct.pack("<" + "I" * len(params), *params))
    return result


def encode_quad(vertices, scale):
    commands = [(0x40, (1,))]
    for position, uv, color, normal in vertices:
        coords = [round(c / scale * 4096) for c in position]
        if not all(-32768 <= c <= 32767 for c in coords):
            raise ValueError("Extension exceeds VTX_16 range")
        tex = [round(c * 16) for c in uv]
        if not all(-32768 <= c <= 32767 for c in tex):
            raise ValueError("Extension exceeds texture-coordinate range")
        commands += [(0x20, (color,)), (0x21, (normal,)),
                     (0x22, ((tex[0] & 0xFFFF) | ((tex[1] & 0xFFFF) << 16),)),
                     (0x23, ((coords[0] & 0xFFFF) | ((coords[1] & 0xFFFF) << 16),
                             coords[2] & 0xFFFF))]
    commands.append((0x41, ()))
    return pack_commands(commands)


def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Expected the unmodified Platinum Iron Island BIN")
    sizes = list(struct.unpack_from("<4I", source))
    start = 16 + sizes[0] + sizes[1]
    model = source[start:start + sizes[2]]
    block = u32(model, 16)
    mdl = block + u32(model, dictionary(model, block + 8)[0][1])
    shape_table = mdl + u32(model, mdl + 12)
    scale = struct.unpack_from("<i", model, mdl + 28)[0] / 4096
    shapes = []
    for name, entry in dictionary(model, shape_table):
        header = shape_table + u32(model, entry)
        dl_start = header + u32(model, header + 8)
        dl = model[dl_start:dl_start + u32(model, header + 12)]
        shapes.append((name, header, dl_start, dl, polygons(dl, scale)))
    additions = [[] for _ in shapes]

    def add(shape, points, uv=None, normal=0x0007FC00, color=0x7FFF):
        uv = uv or [(p[0], p[2]) for p in points]
        additions[shape].append([(tuple(p), tuple(t), color, normal)
                                 for p, t in zip(points, uv)])

    # The original lowest western terrace becomes a narrow strip north of z=-256:
    # its outer 16 units and final 16-unit row were never modelled.
    add(7, [(-192, 32, -256), (-176, 32, -256),
            (-176, 32, -304), (-192, 32, -304)])
    add(7, [(-192, 32, -304), (-112, 32, -304),
            (-112, 32, -320), (-192, 32, -320)])

    def extrusion(shape, polygon, a, b, axis, distance):
        # Continue the original texture gradient along the extrusion axis.
        gradient = (0, 0)
        for other in polygon:
            if (other[0][axis] != a[0][axis]
                    and all(other[0][d] == a[0][d] for d in range(3) if d != axis)):
                difference = a[0][axis] - other[0][axis]
                gradient = tuple((a[1][i] - other[1][i]) / difference for i in range(2))
                break
        def shifted(vertex):
            p = list(vertex[0])
            p[axis] += distance
            return (tuple(p), tuple(vertex[1][i] + gradient[i] * distance for i in range(2)),
                    vertex[2], vertex[3])
        additions[shape].append([b, a, shifted(a), shifted(b)])

    # Carry the west cliff and shoreline up to the completed terrace's north end.
    for shape in (0, 11):
        for polygon in shapes[shape][4]:
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                if a[0][2] == b[0][2] == -256 and max(a[0][0], b[0][0]) < -176:
                    extrusion(shape, polygon, a, b, 2, -64)

    # Continue the exact eastern terrace and shoreline profile for four tiles.
    # Then close its exposed sides with the island's existing cliff material.
    eastern_edge = 320
    eastern_shapes = (0, 7, 8, 11)
    edge_counts = Counter()
    for shape in eastern_shapes:
        for polygon in shapes[shape][4]:
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                edge_counts[tuple(sorted((a[0], b[0])))] += 1
    for shape in eastern_shapes:
        for polygon in shapes[shape][4]:
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                if (a[0][0] == b[0][0] == 256 and a[0] != b[0]
                        and edge_counts[tuple(sorted((a[0], b[0])))] == 1):
                    extrusion(shape, polygon, a, b, 0, eastern_edge - 256)
                    north, south = sorted((a[0], b[0]), key=lambda p: p[2])
                    if north[2] != south[2]:
                        add(0, [(eastern_edge, north[1], north[2]), (eastern_edge, south[1], south[2]),
                                (eastern_edge, 8, south[2]), (eastern_edge, 8, north[2])],
                            [(north[2] * 2, -north[1] * 2), (south[2] * 2, -south[1] * 2),
                             (south[2] * 2, -16), (north[2] * 2, -16)], normal=0x1FF)

    # A local ocean apron also covers the parts outside the original four loaded
    # chunks. Coordinates line up with the repeating original 16x16 sea texture.
    # Keep water out of the extended land's footprint, rather than depending
    # on depth ordering between overlapping scenery and the ocean apron.
    for x0, z0, x1, z1 in [(-480, -480, -256, 480),
                           (eastern_edge, -480, 480, 480),
                           (256, -480, eastern_edge, -256),
                           (256, 244, eastern_edge, 480),
                           (-256, -480, 256, -256), (-256, 256, 256, 480)]:
        # Match the original ocean's 128-unit cells and keep depth interpolation
        # local, especially near the shoreline.
        for x in range(x0, x1, 128):
            for z in range(z0, z1, 128):
                east, south = min(x + 128, x1), min(z + 128, z1)
                add(6, [(x, 8, south), (east, 8, south), (east, 8, z), (x, 8, z)])

    # Repack only the display-list tail; dictionaries/materials/nodes stay put.
    result = bytearray(model[:min(s[2] for s in shapes)])
    for (_, header, _, dl, _), extra in zip(shapes, additions):
        extended_dl = dl + b"".join(encode_quad(q, scale) for q in extra)
        if extra:
            # Added commands use normals, colors and texture coordinates.
            struct.pack_into("<I", result, header + 4, u32(model, header + 4) | 7)
        struct.pack_into("<II", result, header + 8, len(result) - header, len(extended_dl))
        result.extend(extended_dl)
    quad_count = sum(map(len, additions))
    for offset, delta in ((36, quad_count * 4), (38, quad_count), (42, quad_count)):
        struct.pack_into("<H", result, mdl + offset, u16(model, mdl + offset) + delta)
    struct.pack_into("<I", result, 8, len(result))
    struct.pack_into("<I", result, block + 4, len(result) - block)
    struct.pack_into("<I", result, mdl, len(result) - mdl)
    struct.pack_into("<I", result, mdl + 16, len(result) - mdl)  # no envelope matrices
    box_scale = struct.unpack_from("<i", model, mdl + 56)[0] / 4096
    struct.pack_into("<6h", result, mdl + 44,
                     *(round(c / box_scale * 4096) for c in (-480, 8, -480, 960, 208, 960)))
    if len(result) > 0xF000:
        raise ValueError("Extended model exceeds the engine's model buffer")
    sizes[2] = len(result)
    return (struct.pack("<4I", *sizes) + source[16:start] + result
            + source[start + u32(source, 8):]), quad_count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result, count = extend(args.input.read_bytes())
    args.output.write_bytes(result)
    print(f"Iron Island: added {count} scenery quads; model {u32(result, 8)} bytes")


if __name__ == "__main__":
    main()
