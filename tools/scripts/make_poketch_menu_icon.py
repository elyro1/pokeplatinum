#!/usr/bin/env python3
"""Derive menu animation and palettes from existing upstream resources."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('animation', 'menu_palette', 'item_palette', 'output_animation', 'output_palette'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    animation = json.loads(args.animation.read_text())
    animation['sequences'] = animation['sequences'][:3]
    animation['animationResults'] = animation['animationResults'][:10]
    for result in animation['animationResults']:
        result['index'] = 0
    animation.update(sequenceCount=3, frameCount=18, resultCount=10,
                     labels=['radar_none', 'radar_swell', 'radar_wiggle'], labelCount=3)
    args.output_animation.write_text(json.dumps(animation, indent=4) + '\n')
    menu = args.menu_palette.read_text().splitlines()[3:]
    item = args.item_palette.read_text().splitlines()[3:19]
    # Match the menu's inactive shades, followed by the item's active colors.
    shades = [menu[i] for i in (0, 1, 3, 8, 5, 8, 2, 5, 2, 8, 5, 0, 5, 5, 1, 1)]
    args.output_palette.write_bytes(('\r\n'.join(['JASC-PAL', '0100', '32'] + shades + item) + '\r\n').encode())


if __name__ == '__main__':
    main()
