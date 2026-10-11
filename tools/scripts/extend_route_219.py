#!/usr/bin/env python3
"""Continue Route 219's decorative forest and coast for port widescreen views."""

import argparse
import hashlib
import json
from pathlib import Path

from map_scenery import read_land, repack_land, u32


SOURCE_SHA256 = "58f383773d03af0007fac31f6a9c22ffc257794f167755487b8a3cb93288c3f0"


def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Expected the original Platinum Route 219 BIN")
    _, _, _, _, shapes = read_land(source)
    additions = [[] for _ in shapes]

    def stretch(shape, polygon, a, b, distance):
        gradient = (0, 0)
        for other in polygon:
            if (other[0][0] != a[0][0] and other[0][1:] == a[0][1:]):
                delta = a[0][0] - other[0][0]
                gradient = tuple((a[1][i] - other[1][i]) / delta for i in range(2))
                break

        def shifted(vertex):
            p, uv, color, normal = vertex
            return ((p[0] + distance, p[1], p[2]),
                    tuple(uv[i] + gradient[i] * distance for i in range(2)), color, normal)

        additions[shape].append([b, a, shifted(a), shifted(b)])

    # Continue the existing cliff, terrace, forest, shadow and shore profiles.
    # Forest panels already repeat their texture across multiple tree columns;
    # continue that UV gradient instead of stretching a single tree silhouette.
    for shape in (0, 2, 6, 7, 10, 12):
        for polygon in shapes[shape][4]:
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                if a[0][0] == b[0][0] and abs(a[0][0]) == 256:
                    stretch(shape, polygon, a, b, 224 if a[0][0] > 0 else -224)

    # Grass beneath the continued forest; the cliff begins at its south edge.
    for x0, x1, south in [(-480, -256, -160), (256, 480, -96)]:
        points = [(x0, 16, south), (x1, 16, south),
                  (x1, 16, -256), (x0, 16, -256)]
        additions[3].append([(p, (p[0], p[2]), 0x7FFF, 0x0007FC00) for p in points])

    return repack_land(source, additions, (-480, 8, -258, 960, 51, 514))


def extend_matrix(source):
    matrix = json.loads(source)
    # Only the two decorative margins beside Route 219 change. Their original
    # grass plane is at y=16 and would cover the continued shoreline and sea.
    for x in (4, 6):
        if (matrix["maps"][27][x] != "MAP_173"
                or matrix["altitudes"][27][x] != 2
                or matrix["headers"][27][x] != "MAP_HEADER_EVERYWHERE"):
            raise ValueError("Unexpected Route 219 matrix margin")
        matrix["maps"][27][x] = "MAP_174"
        matrix["altitudes"][27][x] = 1
    return json.dumps(matrix, indent=4) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", action="store_true")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.matrix:
        args.output.write_text(extend_matrix(args.input.read_text()))
    else:
        result, count = extend(args.input.read_bytes())
        args.output.write_bytes(result)
        print(f"Route 219: added {count} scenery quads; model {u32(result, 8)} bytes")


if __name__ == "__main__":
    main()
