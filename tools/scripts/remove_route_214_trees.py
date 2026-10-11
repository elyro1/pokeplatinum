#!/usr/bin/env python3
"""Replace the western Route 214 forest filler with the adjoining rock plateau.

An inaccessible matrix margin changes and generated chunk 154 closes its
exposed cliff side. Original assets and gameplay sections remain intact.
"""
import argparse
import hashlib
import json
import math
import struct
from map_scenery import read_land, repack_land, u32
from pathlib import Path

SOURCE_HASHES = (
    "8ed71dbb60d3eaa6fb17c6abc3a60a588bbab71e52d35c3007b783ff49e1c6f7",
    "428e1e5c98de25a4f98ac65ddf9ba4966b1e8a1cb2e9efbac85acc13b4443bee",
)


def remove_trees(source, rock, forest):
    for data, expected in zip((rock, forest), SOURCE_HASHES):
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError("Expected the original Platinum filler BINs")
    matrix = json.loads(source)
    expected_cells = {
        (22, 20): ("MAP_152", 0, "MAP_HEADER_ROUTE_214"),
        (22, 21): ("MAP_153", 0, "MAP_HEADER_ROUTE_214"),
        (22, 22): ("MAP_154", 0, "MAP_HEADER_ROUTE_214"),
        (21, 21): ("MAP_175", 8, "MAP_HEADER_EVERYWHERE"),
        (21, 22): ("MAP_176", 2, "MAP_HEADER_EVERYWHERE"),
        (21, 23): ("MAP_155", 0, "MAP_HEADER_VALOR_LAKEFRONT"),
    }
    for (x, z), expected in expected_cells.items():
        if tuple(matrix[key][z][x] for key in ("maps", "altitudes", "headers")) != expected:
            raise ValueError(f"Unexpected Route 214 matrix cell ({x}, {z})")
    # Matrix altitude is in 8-unit steps: continue the northern Y=64 rock
    # filler and the southern route chunk's western Y=64 plateau.
    matrix["maps"][22][21] = "MAP_175"
    matrix["altitudes"][22][21] = 8
    return json.dumps(matrix, indent=4) + "\n"


LAND_SHA256 = "5697bd8f4a3a354d22866938eeec01ab22a369864195e8e6e156adafaabb6076"


def close_cliff(source):
    if hashlib.sha256(source).hexdigest() != LAND_SHA256:
        raise ValueError("Expected the original Route 214 southern BIN")
    model, _, mdl, _, shapes = read_land(source)
    additions = [[] for _ in shapes]
    # Inspected SBC: shape 5 uses criffp_lm47. Continue the filler side down
    # to the authored western profile, including its rounded terrace lips.
    profile = [(-80, 64), (-76, 62), (-68, 50), (-64, 48),
               (-16, 48), (-12, 46), (-4, 34), (0, 32),
               (16, 32), (20, 30), (28, 18), (32, 16), (256, 16)]
    for (z0, y0), (z1, y1) in zip(profile, profile[1:]):
        points = [(-256, 64, z0), (-256, 64, z1),
                  (-256, y1, z1), (-256, y0, z0)]
        additions[5].append([(p, (p[2] * 2, (64 - p[1]) * 2), 0x7FFF, 0x1FF)
                              for p in points])
    # Close the visible southern end of the filler beside Valor Lakefront.
    points = [(-480, 64, 256), (-256, 64, 256),
              (-256, 16, 256), (-480, 16, 256)]
    additions[5].append([(p, (p[0] * 2, (64 - p[1]) * 2), 0x7FFF, 0x1FF00000)
                          for p in reversed(points)])
    points = [v[0] for sh in shapes for polygon in sh[4] for v in polygon]
    points += [v[0] for extra in additions for polygon in extra for v in polygon]
    quantum = struct.unpack_from("<i", model, mdl + 56)[0] / 4096 / 4096
    low = [math.floor(min(p[i] for p in points) / quantum) * quantum for i in range(3)]
    high = [math.ceil(max(p[i] for p in points) / quantum) * quantum for i in range(3)]
    return repack_land(source, additions, low + [high[i] - low[i] for i in range(3)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--land", action="store_true")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("rock", type=Path, nargs="?")
    parser.add_argument("forest", type=Path, nargs="?")
    args = parser.parse_args()
    if args.land:
        result, count = close_cliff(args.input.read_bytes())
        args.output.write_bytes(result)
        print(f"Route 214: {count} cliff side quads; model {u32(result, 8)} bytes")
        return
    args.output.write_text(remove_trees(args.input.read_text(), args.rock.read_bytes(), args.forest.read_bytes()))
    print("Route 214: western forest filler replaced with Y=64 rock")


if __name__ == "__main__":
    main()
