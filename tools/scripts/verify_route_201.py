#!/usr/bin/env python3
"""Independently check Route 201's generated land model and archive delta.

Usage: verify_route_201.py original.bin generated.bin before.narc after.narc
The baseline archive must contain the same other port scenery edits.
"""
import argparse
import struct
from pathlib import Path


def num(data, offset, fmt='I'):
    return struct.unpack_from('<' + fmt, data, offset)[0]


def entries(data, offset):
    start = offset + num(data, offset + 6, 'H')
    return [start + 4 + i * num(data, start, 'H') for i in range(data[offset + 1])]


def model(land):
    sizes = struct.unpack_from('<4I', land)
    start = 16 + sizes[0] + sizes[1]
    data = land[start:start + sizes[2]]
    assert data[:4] == b'BMD0' and num(data, 8) == len(data)
    block = num(data, 16)
    assert data[block:block + 4] == b'MDL0'
    assert num(data, block + 4) == len(data) - block
    mdl = block + num(data, entries(data, block + 8)[0])
    assert num(data, mdl) == len(data) - mdl
    assert num(data, mdl + 16) == len(data) - mdl
    table = mdl + num(data, mdl + 12)
    lists = []
    for entry in entries(data, table):
        header = table + num(data, entry)
        dl = header + num(data, header + 8)
        end = dl + num(data, header + 12)
        assert table <= header < dl < end <= len(data)
        lists.append(data[dl:end])
    return data, mdl, lists, start


def added_vertices(dl, scale):
    # This decoder is independent of map_scenery. All appended quads encode
    # every vertex attribute explicitly, so no inherited GPU state is needed.
    arity = {0: 0, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2, 0x40: 1, 0x41: 0}
    vertices, cursor, count, in_quad = [], 0, 0, False
    while cursor < len(dl):
        ops = dl[cursor:cursor + 4]
        cursor += 4
        for op in ops:
            n = arity[op]
            args = struct.unpack_from('<' + 'I' * n, dl, cursor)
            cursor += n * 4
            if op == 0x40:
                assert args == (1,) and not in_quad
                in_quad, count = True, 0
            elif op == 0x23:
                assert in_quad
                raw = struct.pack('<II', *args)
                vertices.append(tuple(v * scale / 4096 for v in struct.unpack_from('<3h', raw)))
                count += 1
            elif op == 0x41:
                assert in_quad and count == 4
                in_quad = False
    assert cursor == len(dl) and not in_quad
    return vertices


def members(data):
    assert data[:4] == b'NARC' and num(data, 8) == len(data)
    cursor, fat, payload = 16, None, None
    for _ in range(num(data, 14, 'H')):
        size = num(data, cursor + 4)
        if data[cursor:cursor + 4] == b'BTAF':
            fat = [struct.unpack_from('<II', data, cursor + 12 + 8 * i)
                   for i in range(num(data, cursor + 8, 'H'))]
        if data[cursor:cursor + 4] == b'GMIF':
            payload = data[cursor + 8:cursor + size]
        cursor += size
    assert cursor == len(data) and fat is not None and payload is not None
    return [payload[a:b] for a, b in fat]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'generated', 'before', 'after'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    old, new = args.original.read_bytes(), args.generated.read_bytes()
    a, am, ads, start = model(old)
    b, bm, bds, bstart = model(new)
    assert start == bstart and old[16:start] == new[16:start]
    assert old[start + len(a):] == new[start + len(b):]  # BDHC, padding
    assert len(b) < 0xF000 and len(ads) == len(bds) == 13
    # Compare nodes, SBC, material dictionaries and shape records, allowing
    # only the documented size/count/bounds and display-list offset fields.
    table = am + num(a, am + 12)
    headers = [table + num(a, entry) for entry in entries(a, table)]
    tail = min(h + num(a, h + 8) for h in headers)
    prefix = bytearray(b[:tail])
    fields = [(8, 4), (num(a, 16) + 4, 4), (am, 4), (am + 16, 4),
              (am + 36, 4), (am + 42, 2), (am + 44, 12)]
    fields += [(h + 8, 8) for h in headers]
    for i in (7, 8, 10):
        assert num(b, headers[i] + 4) == num(a, headers[i] + 4) | 7
        fields.append((headers[i] + 4, 4))
    for offset, length in fields:
        prefix[offset:offset + length] = a[offset:offset + length]
    assert bytes(prefix) == a[:tail]
    changed, vertices = [], []
    scale = num(b, bm + 28, 'i') / 4096
    for i, (x, y) in enumerate(zip(ads, bds)):
        assert y.startswith(x)  # original GPU command stream unchanged
        if x != y:
            changed.append(i)
            vertices += added_vertices(y[len(x):], scale)
    assert changed == [7, 8, 10] and len(vertices) == 104
    for offset, delta in ((36, 104), (38, 26), (42, 26)):
        assert num(b, bm + offset, 'H') == num(a, am + offset, 'H') + delta
    box = struct.unpack_from('<6h', b, bm + 44)
    unit = num(b, bm + 56, 'i') / 4096 / 4096
    assert all(box[d] * unit <= p[d] <= (box[d] + box[d + 3]) * unit
               for p in vertices for d in range(3))
    before, after = members(args.before.read_bytes()), members(args.after.read_bytes())
    assert len(before) == len(after) == 666
    assert [i for i, (x, y) in enumerate(zip(before, after)) if x != y] == [6]
    assert after[6][:len(new)] == new
    print('PASS: only member 6 / model shapes 7,8,10 changed; 26 quads; '
          'original display lists, gameplay sections, bounds and counts verified')


if __name__ == '__main__':
    main()
