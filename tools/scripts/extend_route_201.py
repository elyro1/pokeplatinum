#!/usr/bin/env python3
"""Close Route 201's two-column decorative forest gap above Route 219."""
import argparse
import hashlib
from pathlib import Path
from map_scenery import read_land, repack_land, u32

SOURCE_SHA256 = '45e56ba75cc8ff8c4adae81cc3456c37c966504a5cb8246c864f663f1c2fc409'


def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError('Expected the original Route 201 eastern BIN')
    _, _, _, _, shapes = read_land(source)
    additions = [[] for _ in shapes]
    # Inspected SBC: shape 7 binds tree04_2_lm3; shape 10 binds
    # ngrass_lm26. Copy all four layers of the southern tree centred at X=16.
    # In matrix cell (4,27), Twinleaf stops at X=-32 and Route 219 starts
    # at X=32. This chunk remains loaded for either horizontal loading pair.
    tree = [p for p in shapes[7][4]
            if min(v[0][2] for v in p) > 216
            and (min(v[0][0] for v in p) + max(v[0][0] for v in p)) / 2 == 16]
    if len(tree) != 4:
        raise ValueError('Unexpected Route 201 southern tree layers')
    for dx in (-32, 0):
        for dz in (32, 64, 96):
            for polygon in tree:
                additions[7].append([((p[0] + dx, p[1], p[2] + dz), uv, color, normal)
                                     for p, uv, color, normal in polygon])
    points = [(-32, 16, 352), (32, 16, 352), (32, 16, 256), (-32, 16, 256)]
    additions[10].append([(p, (p[0], p[2]), 0x7FFF, 0x0007FC00) for p in points])
    # Close the small southern end down to sea level. Shape 8 is the existing
    # sand-edge material nsandp_lm57, using its authored lower dirt texels.
    points = [(-32, 16, 352), (-32, 8, 352), (32, 8, 352), (32, 16, 352)]
    additions[8].append([(p, uv, 0x7FFF, 0x1FF00000) for p, uv in
                          zip(points, [(16, 16.25), (16, 24), (80, 24), (80, 16.25)])])
    return repack_land(source, additions, (-260, 8, -258, 520, 51, 610))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result, count = extend(args.input.read_bytes())
    args.output.write_bytes(result)
    print(f'Route 201: added {count} scenery quads; model {u32(result, 8)} bytes')


if __name__ == '__main__':
    main()
