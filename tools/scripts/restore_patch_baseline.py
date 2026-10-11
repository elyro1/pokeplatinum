#!/usr/bin/env python3
"""Reconstruct pristine sources in the build directory from change-only hunks."""
import argparse
import hashlib
from pathlib import Path
import re


def restore(source, patch):
    lines = source.decode('utf-8').splitlines(keepends=True)
    diff = patch.decode('utf-8').splitlines(keepends=True)
    prefix = '# original-sha256: '
    if not diff or not diff[0].startswith(prefix):
        raise ValueError('Missing pristine source hash')
    digest = diff[0][len(prefix):].strip()
    result, cursor, i = [], 0, 1
    while i < len(diff):
        if diff[i].startswith(('--- ', '+++ ')):
            i += 1
            continue
        match = re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@[^\n]*\n', diff[i])
        if not match:
            raise ValueError('Invalid baseline patch hunk')
        start, count, new_start, new_count = (
            int(value) if value is not None else 1 for value in match.groups())
        start = start - 1 if count else start
        if start < cursor or start > len(lines):
            raise ValueError('Invalid baseline patch offset')
        result.extend(lines[cursor:start])
        cursor, consumed, produced = start, 0, 0
        if len(result) != (new_start - 1 if new_count else new_start):
            raise ValueError('Invalid baseline output offset')
        i += 1
        while i < len(diff) and not diff[i].startswith('@@ '):
            op, value = diff[i][:1], diff[i][1:]
            if op not in (' ', '-', '+'):
                raise ValueError('Invalid baseline patch operation')
            if op in (' ', '-'):
                if cursor >= len(lines) or lines[cursor] != value:
                    raise ValueError('Source changed; update its baseline patch')
                cursor += 1
                consumed += 1
            if op in (' ', '+'):
                result.append(value)
                produced += 1
            i += 1
        if (consumed, produced) != (count, new_count):
            raise ValueError('Invalid baseline patch line counts')
    result.extend(lines[cursor:])
    data = ''.join(result).encode('utf-8')
    if hashlib.sha256(data).hexdigest() != digest:
        raise ValueError('Pristine source hash mismatch; update its baseline patch')
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'patch', 'output'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    args.output.write_bytes(restore(args.source.read_bytes(), args.patch.read_bytes()))


if __name__ == '__main__':
    main()
