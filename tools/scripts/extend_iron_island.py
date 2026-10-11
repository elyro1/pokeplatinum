#!/usr/bin/env python3
"""Complete Iron Island's decorative terrain for the PC widescreen camera.

Read the original Platinum land BIN and extend its existing display lists.
No model converter is needed. Permissions, props, BDHC, materials and original
polygons are preserved; only the model and its bounds/counts change.
This intentionally supports one known static model, not arbitrary NSBMD files.
"""

import argparse
import hashlib
from collections import Counter
from pathlib import Path

from map_scenery import read_land, repack_land, u32


SOURCE_SHA256 = "9c000bcd8f0bbd80d0e377952c5e9cd35a6986f71453fb84c18b1a1e21ddfc2b"

def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Expected the unmodified Platinum Iron Island BIN")
    _, _, _, _, shapes = read_land(source)
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

    return repack_land(source, additions, (-480, 8, -480, 960, 208, 960))


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
