#!/usr/bin/env python3
"""Replace Hearthome's elevated western rock filler with its existing forest.

The city spans MAP_057..060. Only the inaccessible margin at matrix (13,21)
changes; copying trees into MAP_057 would intersect the Y=128 rock filler.
Reuse MAP_176 at altitude 4 (Y=32), matching the city's forest floor and the
northern filler row. No land BIN or shared texture needs modification.
"""

import argparse
import hashlib
import json
from pathlib import Path

SOURCE_HASHES = (
    '8ed71dbb60d3eaa6fb17c6abc3a60a588bbab71e52d35c3007b783ff49e1c6f7',
    '428e1e5c98de25a4f98ac65ddf9ba4966b1e8a1cb2e9efbac85acc13b4443bee',
)


def extend_matrix(source, rock, forest):
    for data, expected in zip((rock, forest), SOURCE_HASHES):
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError('Expected the original Platinum decorative filler BINs')
    matrix = json.loads(source)
    expected_cells = {
        (13, 21): ('MAP_175', 16, 'MAP_HEADER_EVERYWHERE'),
        (13, 20): ('MAP_176', 4, 'MAP_HEADER_EVERYWHERE'),
        (14, 21): ('MAP_057', 0, 'MAP_HEADER_HEARTHOME_CITY'),
        (14, 22): ('MAP_059', 0, 'MAP_HEADER_HEARTHOME_CITY'),
        (13, 22): ('MAP_056', 0, 'MAP_HEADER_ROUTE_208'),
    }
    for (x, z), expected in expected_cells.items():
        if tuple(matrix[key][z][x] for key in ('maps', 'altitudes', 'headers')) != expected:
            raise ValueError(f'Unexpected Hearthome matrix cell ({x}, {z})')
    matrix['maps'][21][13] = 'MAP_176'
    matrix['altitudes'][21][13] = 4
    return json.dumps(matrix, indent=4) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('rock', type=Path)
    parser.add_argument('forest', type=Path)
    args = parser.parse_args()
    args.output.write_text(extend_matrix(args.input.read_text(), args.rock.read_bytes(), args.forest.read_bytes()))
    print('Hearthome City: western decorative rock filler replaced with matching forest')


if __name__ == '__main__':
    main()
