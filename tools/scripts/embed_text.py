#!/usr/bin/env python3
"""Embed a text file as a C string: embed_text.py INPUT OUTPUT SYMBOL."""
import sys

source, output, symbol = sys.argv[1:4]
text = open(source, encoding='utf-8').read()
with open(output, 'w', encoding='utf-8') as out:
    out.write(f'// Generated from {source.rsplit("/", 1)[-1]} by embed_text.py; do not edit.\n')
    out.write(f'static const char {symbol}[] =\n')
    for line in text.splitlines(keepends=True):
        escaped = line.encode('utf-8').decode('utf-8').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
        out.write(f'    "{escaped}"\n')
    out.write('    "";\n')
