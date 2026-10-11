#!/usr/bin/env python3
"""Test the release patcher against original and development archive members.

No ROM is needed: original members come from the source tree. All writes are
isolated in a temporary directory; the development game's files are untouched.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from make_map_patches import LAND, MATRIX, ROOT
from verify_hearthome_scenery import members


def repack(template, files):
    data = template.read_bytes()
    header_size = struct.unpack_from('<H', data, 12)[0]
    blocks, offset = [], header_size
    while offset < len(data):
        size = struct.unpack_from('<I', data, offset + 4)[0]
        blocks.append(bytearray(data[offset:offset + size]))
        offset += size
    fat = next(b for b in blocks if b[:4] == b'BTAF')
    image = next(b for b in blocks if b[:4] == b'GMIF')
    image[8:] = b''
    for i, member in enumerate(files):
        start = len(image) - 8
        image.extend(member)
        struct.pack_into('<II', fat, 12 + i * 8, start, len(image) - 8)
        image.extend(b'\xff' * (-len(image) % 4))
    struct.pack_into('<I', image, 4, len(image))
    result = bytearray(data[:header_size]) + b''.join(blocks)
    struct.pack_into('<I', result, 8, len(result))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT / 'build_linux')
    args = parser.parse_args()
    build = args.build.resolve()
    land = build / 'res/field/maps/data/land_data.narc'
    matrix = build / 'res/field/matrices/map_matrix.narc'
    expected = {LAND: members(land), MATRIX: members(matrix)}
    originals = {LAND: [(ROOT / 'res/field/maps/data' / name).read_bytes()
                       for name in (ROOT / 'res/field/maps/data/map_data.order').read_text().splitlines()],
                 MATRIX: list(expected[MATRIX])}
    with tempfile.TemporaryDirectory(prefix='map-patches-') as temp:
        work = Path(temp)
        original_matrix = work / 'matrix.bin'
        subprocess.run(['python3', str(ROOT / 'tools/jsoncnv/map_matrix.py'),
                        str(ROOT / 'res/field/matrices/map_matrix_000.json'), str(original_matrix)],
                       env=dict(os.environ, PYTHONPATH=str(build)), check=True)
        originals[MATRIX][0] = original_matrix.read_bytes()
        subprocess.run(['python3', str(ROOT / 'tools/scripts/make_map_patches.py'),
                        str(build), str(land), str(matrix), str(work / 'map_patches.inc')], check=True)
        binary = work / 'patcher'
        subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-DMAP_PATCH_TEST',
                        '-I' + str(ROOT / 'include'), '-I' + str(work),
                        str(ROOT / 'src/port/map_patches.cpp'), '-o', str(binary)], check=True)
        def run(directory, success=True):
            result = subprocess.run([str(binary)], cwd=directory, capture_output=True, text=True)
            assert (result.returncode == 0) == success, result.stderr
            return result.stderr
        def populate(directory, data):
            directory.mkdir()
            for path, files in data.items():
                target = directory / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(repack(land if path == LAND else matrix, files))
        clean = work / 'clean'
        populate(clean, originals)
        run(clean)
        for path in expected:
            assert members(clean / path) == expected[path], path
            assert members(clean / '.map-patch-originals' / path) == originals[path]
        print('PASS: all patched and untouched members match the development build')
        before = {path: (clean / path).stat().st_mtime_ns for path in expected}
        run(clean)
        assert before == {path: (clean / path).stat().st_mtime_ns for path in expected}
        print('PASS: repeat startup performs no writes')
        # Simulate an older release's output. Upgrade must use pristine backups,
        # while keeping a player's change to an unrelated member.
        changed = members(clean / LAND)
        changed[0] = b'older release output'
        changed[1] = b'unrelated mod'
        (clean / LAND).write_bytes(repack(land, changed))
        run(clean)
        changed = members(clean / LAND)
        assert changed[0] == expected[LAND][0] and changed[1] == b'unrelated mod'
        print('PASS: upgrades use original backups and preserve unrelated members')
        dev = work / 'dev'
        populate(dev, expected)
        run(dev)
        assert not (dev / '.map-patch-originals').exists()
        print('PASS: development assets are recognized without backups or rewriting')
        corrupt = work / 'corrupt'
        bad = {path: list(files) for path, files in originals.items()}
        bad[MATRIX][0] = b'wrong ROM member'
        populate(corrupt, bad)
        before = {path: (corrupt / path).read_bytes() for path in expected}
        assert 'Original map data unavailable' in run(corrupt, False)
        assert before == {path: (corrupt / path).read_bytes() for path in expected}
        print('PASS: unknown assets fail without replacing any game archives')
        missing = work / 'missing'
        missing.mkdir()
        assert 'Cannot read' in run(missing, False)
        print('PASS: missing filesystem fails clearly in the standalone patcher')


if __name__ == '__main__':
    main()
