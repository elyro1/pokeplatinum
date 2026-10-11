#!/usr/bin/env python3
"""Complete Route 207's western rock terrace without changing playable land."""
import argparse
import hashlib
import json
from pathlib import Path

from map_scenery import read_land, repack_land, u32

SOURCE_SHA256 = "45ed6b4d409dffaaa04be57e851e790719b92e3b3ec95fc82172e2e49c97b5eb"
FILLER_SHA256 = "8ed71dbb60d3eaa6fb17c6abc3a60a588bbab71e52d35c3007b783ff49e1c6f7"


def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Expected the original Platinum Route 207 west BIN")
    _, _, _, _, shapes = read_land(source)
    assert len(shapes) == 15
    additions = [[] for _ in shapes]
    # SBC binds polygon4 to criff_lm44 (flat rock), polygon2 to criffp_lm42
    # (rounded cliff). These are this model's bindings, not Iron Island's.
    west = -480

    def flat(x0, z0, x1, z1, y):
        additions[4].append([((x, y, z), (x, z), 0x7FFF, 0x0007FC00)
                             for x, z in [(x0, z1), (x1, z1), (x1, z0), (x0, z0)]])

    # 1:1 repeating rock texture, partitioned like the original 128-unit cells.
    # The filler is lowered to Y=96. Never add a coplanar apron at that height.
    for x in range(west, -256, 128):
        for z in range(-256, 192, 128):
            flat(x, z, min(x + 128, -256), min(z + 128, 192), 112)

    # Copy the actual southern rounded boundary profile, including the two
    # triangle edges at the corner. Keep its UVs, per-vertex normals and colors;
    # the criffp atlas repeats every 32 texels horizontally. Two texels/unit
    # matches the adjacent cliff's authored mapping. Reverse the shared edge
    # to preserve the source mesh's face winding.
    profile = []
    for polygon in shapes[2][4]:
        for a, b in zip(polygon, polygon[1:] + polygon[:1]):
            if a[0][0] == b[0][0] == -256 and a[0][2] >= 192 and b[0][2] >= 192:
                profile.append((a, b))
    if len(profile) != 3:
        raise ValueError("Unexpected western cliff profile")
    for x in range(-256, west, -112):
        def shifted(v, dx):
            p, uv, color, normal = v
            return ((p[0] + dx, p[1], p[2]), (uv[0] + dx * 2, uv[1]), color, normal)

        for a, b in profile:
            additions[2].append([shifted(b, x + 256), shifted(a, x + 256),
                                 shifted(a, x + 144), shifted(b, x + 144)])

    # Close the outer end down to the lower filler. Use the existing rounded
    # cliff material; no water or exposed vertical hole beneath the extension.
    for z in range(-256, 192, 112):
        z1 = min(z + 112, 192)
        additions[2].append([((west, y, zz), (zz * 2, (112 - y) * 2), 0x7FFF, 0x201)
                            for y, zz in [(112, z1), (112, z), (96, z), (96, z1)]])
    # Close the tapered end of the rounded southern lip as well. Its last
    # quad collapses to a triangle at Y=96, where it meets the filler.
    for a, b in profile:
        north, south = sorted((a[0], b[0]), key=lambda p: p[2])
        additions[2].append([
            ((west, y, z), (z * 2, (112 - y) * 2), 0x7FFF, 0x201)
            for y, z in [(south[1], south[2]), (north[1], north[2]),
                         (96, north[2]), (96, south[2])]
        ])
    return repack_land(source, additions, (-480, 48, -258, 736, 96, 514))


def extend_matrix(source, filler):
    if hashlib.sha256(filler).hexdigest() != FILLER_SHA256:
        raise ValueError("Expected the original rock filler BIN")
    matrix = json.loads(source)
    if (matrix["maps"][22][8] != "MAP_175" or matrix["altitudes"][22][8] != 14
            or matrix["headers"][22][8] != "MAP_HEADER_EVERYWHERE"):
        raise ValueError("Unexpected Route 207 western margin")
    matrix["altitudes"][22][8] = 12
    return json.dumps(matrix, indent=4) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", type=Path, help="original rock filler BIN")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.matrix:
        args.output.write_text(extend_matrix(args.input.read_text(), args.matrix.read_bytes()))
    else:
        result, count = extend(args.input.read_bytes())
        args.output.write_bytes(result)
        print(f"Route 207: added {count} scenery quads; model {u32(result, 8)} bytes")


if __name__ == "__main__":
    main()
