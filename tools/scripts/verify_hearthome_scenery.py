#!/usr/bin/env python3
"""Independently compare the pre/post-build archives for Hearthome scenery.

Usage: verify_hearthome_scenery.py BEFORE_LAND AFTER_LAND BEFORE_MATRIX AFTER_MATRIX
Uses only struct, without importing the generator or map_scenery decoder.
"""
import struct
import sys
from pathlib import Path


def unpack(fmt, data, offset):
    return struct.unpack_from('<' + fmt, data, offset)


def members(path):
    data = Path(path).read_bytes()
    assert data[:4] == b'NARC'
    assert unpack('I', data, 8)[0] == len(data)
    offset = unpack('H', data, 12)[0]
    ranges = payload = None
    while offset < len(data):
        size = unpack('I', data, offset + 4)[0]
        assert size >= 8 and offset + size <= len(data)
        if data[offset:offset + 4] == b'BTAF':
            ranges = [unpack('II', data, offset + 12 + 8 * i)
                      for i in range(unpack('H', data, offset + 8)[0])]
        elif data[offset:offset + 4] == b'GMIF':
            payload = data[offset + 8:offset + size]
        offset += size
    assert ranges is not None and payload is not None
    assert all(0 <= a <= b <= len(payload) for a, b in ranges)
    return [payload[a:b] for a, b in ranges]


def land_sections(data):
    sizes = unpack('4I', data, 0)
    offset, sections = 16, []
    for size in sizes:
        sections.append(data[offset:offset + size])
        offset += size
    assert offset <= len(data) < offset + 4
    assert sizes[0] == 2048
    return sections


def dictionary_offsets(data, offset):
    entries = offset + unpack('H', data, offset + 6)[0]
    stride = unpack('H', data, entries)[0]
    return [entries + 4 + i * stride for i in range(data[offset + 1])]


def validate_forest(model):
    assert model[:4] == b'BMD0' and unpack('I', model, 8)[0] == len(model)
    assert len(model) == 18912 < 0xF000
    block = unpack('I', model, 16)[0]
    assert model[block:block + 4] == b'MDL0'
    assert block + unpack('I', model, block + 4)[0] == len(model)
    entry, = dictionary_offsets(model, block + 8)
    mdl = block + unpack('I', model, entry)[0]
    assert mdl + unpack('I', model, mdl)[0] == len(model)
    shape_table = mdl + unpack('I', model, mdl + 12)[0]
    shapes = dictionary_offsets(model, shape_table)
    assert len(shapes) == 4
    # Independently walk every packed GX command. Original display lists and
    # model metadata are reused byte-for-byte, so no encoding is regenerated.
    arities = {0: 0, 32: 1, 33: 1, 34: 1, 35: 2, 36: 1, 37: 1,
               38: 1, 39: 1, 40: 1, 64: 1, 65: 0}
    vertices = begins = ends = 0
    for entry in shapes:
        header = shape_table + unpack('I', model, entry)[0]
        start, size = unpack('II', model, header + 8)
        dl = model[header + start:header + start + size]
        assert len(dl) == size
        offset = 0
        while offset < len(dl):
            ops = dl[offset:offset + 4]
            assert len(ops) == 4
            offset += 4
            for op in ops:
                offset += 4 * arities[op]
                vertices += 35 <= op <= 40
                begins += op == 64
                ends += op == 65
                assert offset <= len(dl)
        assert offset == len(dl)
    assert begins == ends and vertices > 0
    print(f'Forest NSBMD: 4 valid display lists, {vertices} vertex commands, {len(model)} bytes')


def main():
    before_land, after_land, before_matrix, after_matrix = map(members, sys.argv[1:])
    assert len(before_land) == len(after_land) == 666
    assert before_land == after_land, 'Land archive members changed'
    assert len(before_matrix) == len(after_matrix) == 289
    assert [i for i in range(289) if before_matrix[i] != after_matrix[i]] == [0]
    a, b = before_matrix[0], after_matrix[0]
    assert a[:4] == bytes((30, 30, 1, 1)) and len(a) == len(b)
    start, count, cell = 5 + a[4], 900, 21 * 30 + 13
    altitude, land_id = start + 2 * count + cell, start + 3 * count + cell * 2
    assert [i for i in range(len(a)) if a[i] != b[i]] == [altitude, land_id]
    assert (a[altitude], b[altitude]) == (16, 4)
    assert (unpack('H', a, land_id)[0], unpack('H', b, land_id)[0]) == (175, 176)
    rock, forest = map(land_sections, (after_land[175], after_land[176]))
    assert rock[0] == forest[0] == b'\0\x80' * 1024
    assert rock[1] == forest[1] == b''
    assert rock[3] == forest[3]
    validate_forest(forest[2])
    print('PASS: all 666 land members unchanged; only two bytes in matrix member 0 changed')
    print('PASS: filler permissions/props/BDHC identical; every filler tile remains blocked')


if __name__ == '__main__':
    main()
