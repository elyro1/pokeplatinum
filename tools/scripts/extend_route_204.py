#!/usr/bin/env python3
"""Continue the scenery beside Route 204's northern Ravaged Path entrance."""

import argparse
import hashlib
import json
from pathlib import Path

from map_scenery import read_land, repack_land, u32


SOURCE_SHA256 = "38a561911c67b18ab54180aa143550952790e4707ea824542074b5df7d87afb7"


def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Expected the original Platinum Route 204 north BIN")
    _, _, _, _, shapes = read_land(source)
    additions = [[] for _ in shapes]

    # Individual trees use different regions of a texture atlas. Extending UVs
    # wraps into unrelated atlas regions and breaks the crowns/trunks. Copy the
    # complete outer column at its original 32-unit spacing, with UVs unchanged.
    for polygon in shapes[10][4]:
        west = min(v[0][0] for v in polygon) < -250
        east = max(v[0][0] for v in polygon) > 250
        if not (west or east):
            continue
        for column in range(1, 8):
            distance = column * (-32 if west else 32)
            additions[10].append([
                ((p[0] + distance, p[1], p[2]), uv, color, normal)
                for p, uv, color, normal in polygon
            ])

    # The continuous forest materials are designed to tile horizontally;
    # carry those and the cliffs/ground to the new columns' outer edge.
    for shape in (0, 1, 2, 3, 13, 14, 15):
        for polygon in shapes[shape][4]:
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                edge = a[0][0]
                if edge != b[0][0] or abs(edge) != 256:
                    continue
                distance = 224 if edge > 0 else -224
                other = next(v for v in polygon
                             if v[0][0] != edge and v[0][1:] == a[0][1:])
                gradient = tuple((a[1][i] - other[1][i]) / (edge - other[0][0])
                                 for i in range(2))

                def shifted(vertex):
                    p, uv, color, normal = vertex
                    return ((p[0] + distance, p[1], p[2]),
                            tuple(uv[i] + gradient[i] * distance for i in range(2)),
                            color, normal)

                additions[shape].append([b, a, shifted(a), shifted(b)])

    # These bounds include the existing north-edge tree overhang and the new
    # outer foliage; collision, props, BDHC and original geometry stay intact.
    return repack_land(source, additions, (-484, 16, -258, 968, 107, 514), model_limit=0x20000)


def extend_matrix(source):
    matrix = json.loads(source)
    # The generic forest overlaps our continued trees even when lowered.
    # Use the flat rocky filler beneath the terraces instead. Both fillers
    # have identical blocked permissions; keep their non-playable headers.
    for x in (4, 6):
        if (matrix["maps"][21][x] != "MAP_173"
                or matrix["altitudes"][21][x] != 10
                or matrix["headers"][21][x] != "MAP_HEADER_EVERYWHERE"):
            raise ValueError("Unexpected Route 204 north matrix margin")
        matrix["maps"][21][x] = "MAP_175"
        matrix["altitudes"][21][x] = 0
    return json.dumps(matrix, indent=4) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", action="store_true")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.matrix:
        args.output.write_text(extend_matrix(args.input.read_text()))
        return
    result, count = extend(args.input.read_bytes())
    args.output.write_bytes(result)
    print(f"Route 204 north: added {count} scenery quads; model {u32(result, 8)} bytes")


if __name__ == "__main__":
    main()
