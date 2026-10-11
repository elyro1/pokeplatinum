#!/usr/bin/env python3
"""Continue Twinleaf Town's eastern decorative forest for port cameras.

Supports only the hash-guarded original static Twinleaf mesh. Copy complete
four-layer trees rather than stretching the foliage texture atlas.
"""

import argparse
import hashlib
from pathlib import Path

from map_scenery import read_land, repack_land, u32

SOURCE_SHA256 = "66effc99d4468641049282f3679caa3e2a14beb637b0960c942d998a81f4a18b"


def extend(source):
    if hashlib.sha256(source).hexdigest() != SOURCE_SHA256:
        raise ValueError("Expected the original Platinum Twinleaf Town BIN")
    _, _, _, _, shapes = read_land(source)
    additions = [[] for _ in shapes]

    def copy_column(shape, center, columns):
        for polygon in shapes[shape][4]:
            west = min(v[0][0] for v in polygon)
            east = max(v[0][0] for v in polygon)
            if (west + east) / 2 != center:
                continue
            for column in columns:
                additions[shape].append([
                    ((p[0] + column * 32, p[1], p[2]), uv, color, normal)
                    for p, uv, color, normal in polygon
                ])

    # The eastern column has 16 trees split between two material shapes.
    # Retain its 32-unit spacing and every UV/color/normal/winding value.
    for shape in (0, 1):
        copy_column(shape, 240, range(1, 8))
    copy_column(11, 240, range(1, 8))  # grass underneath the same footprint

    # Twinleaf already carries two extra rows south into the adjoining chunk.
    # Continue those too; otherwise the camera sees a notch at the south seam.
    copy_column(0, 272, range(1, 7))
    copy_column(11, 272, range(1, 7))

    # Bounds include the original west/north overhangs and copied crown tips.
    # Route 204 already provides 0x20000-byte port model buffers; ARM uses the
    # original BIN and its original 0xF000-byte buffer.
    return repack_land(source, additions,
                       (-260, -16, -272, 744, 74.3125, 592),
                       model_limit=0x20000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result, count = extend(args.input.read_bytes())
    args.output.write_bytes(result)
    print(f"Twinleaf Town: added {count} scenery quads; model {u32(result, 8)} bytes")


if __name__ == "__main__":
    main()
