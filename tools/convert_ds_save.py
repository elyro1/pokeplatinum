#!/usr/bin/env python3
"""Import an original US Platinum DS save into this 64-bit PC port."""
import argparse
import binascii
from pathlib import Path
import struct
import sys

FLASH_SIZE = 0x80000
HALF_SIZE = 0x40000
SIGNATURE = 0x20060623
FOOTER = struct.Struct('<IIIIBxH')
# sizeof each gSaveTable entry on DS (4-byte alignment). Each entry also
# receives 4-(size%4)+4 bytes in SaveTableEntry_BodySize, even if aligned.
DS_SIZES = (
    92, 44, 1424, 1908, 940, 280, 160, 804, 480, 2176, 1932,
    5120, 2484, 32, 2100, 1120, 800, 264, 1120, 1568, 444, 368,
    1004, 5672, 14, 340, 248, 3016, 3744, 2240, 4088, 4904,
    1672, 40, 8, 64, 8, 74184,
)
PC_SIZES = tuple({0: 104, 8: 488, 37: 74192}.get(i, n)
                 for i, n in enumerate(DS_SIZES))


def page_size(n):
    return n + 4 - n % 4 + 4


def block_sizes(sizes):
    return sum(map(page_size, sizes[:37])) + 20, page_size(sizes[37]) + 20


DS_BLOCKS = block_sizes(DS_SIZES)  # 0xCF2C, 0x121E4
PC_BLOCKS = block_sizes(PC_SIZES)  # 0xCF40, 0x121EC


def crc(data):
    return binascii.crc_hqx(data, 0xFFFF)


def normalize(data):
    if len(data) == FLASH_SIZE:
        return data
    if (FLASH_SIZE < len(data) <= FLASH_SIZE + 4096
            and data.endswith(b'|-DESMUME SAVE-|')):
        return data[:FLASH_SIZE]
    # The PC backup backend writes only as far as its last used flash address.
    if sum(PC_BLOCKS) <= len(data) < FLASH_SIZE:
        padded = data.ljust(FLASH_SIZE, b'\xff')
        if all(valid_blocks(padded, PC_BLOCKS)):
            return padded
    raise ValueError('Expected a 512 KiB raw Platinum save (.sav/.bin), or a '
                     'DeSmuME .dsv with its save footer. Savestates are unsupported.')


def valid_blocks(data, sizes):
    result = [[], []]
    for half in (0, HALF_SIZE):
        offset = half
        for block_id, size in enumerate(sizes):
            body = data[offset:offset + size - 20]
            footer = FOOTER.unpack_from(data, offset + size - 20)
            counter, block_counter, stored_size, signature, ident, checksum = footer
            if (stored_size == size and signature == SIGNATURE
                    and ident == block_id and checksum == crc(body)):
                result[block_id].append((counter, block_counter, body))
            offset += size
    return result


def newer(a, b):
    # SaveData_CompareCounters treats the UINT32_MAX -> 0 rollover specially.
    if a == 0 and b == 0xFFFFFFFF:
        return True
    if a == 0xFFFFFFFF and b == 0:
        return False
    return a > b


def select_pair(blocks):
    pairs = [(normal, boxes) for normal in blocks[0] for boxes in blocks[1]
             if normal[0] == boxes[0]]
    if not pairs:
        raise ValueError('No matching, checksum-valid Platinum progress and PC '
                         'storage blocks were found. The save may be damaged, '
                         'from another game, or from an unsupported version.')
    best = pairs[0]
    for pair in pairs[1:]:
        if (newer(pair[0][0], best[0][0])
                or (pair[0][0] == best[0][0]
                    and (newer(pair[0][1], best[0][1])
                         or (pair[0][1] == best[0][1]
                             and newer(pair[1][1], best[1][1]))))):
            best = pair
    return best


def convert_entry(index, source):
    target = bytearray(PC_SIZES[index])
    if index == 0:
        # GameTime: align its two s64 timestamps to 8 and add tail padding.
        target[:52] = source[:52]
        target[56:76] = source[52:72]
        target[80:100] = source[72:92]
    elif index == 8:
        # Two 236-byte DaycareMon records become 240-byte records.
        target[:236] = source[:236]
        target[240:476] = source[236:472]
        target[480:488] = source[472:480]
    elif index == 37:
        # Align the BoxPokemon array; Pokemon bytes and encryption stay intact.
        target[:4] = source[:4]
        target[8:74187] = source[4:74183]
    else:
        target[:] = source
    return target


def convert_body(block_id, source):
    result = bytearray()
    indices = range(37) if block_id == 0 else (37,)
    offset = 0
    for index in indices:
        size = DS_SIZES[index]
        page = convert_entry(index, source[offset:offset + size])
        page.extend(bytes(page_size(PC_SIZES[index]) - len(page)))
        struct.pack_into('<H', page, len(page) - 4, crc(page[:-4]))
        result.extend(page)
        offset += page_size(size)
    return result


def convert_recordings(data, output):
    converted = 0
    # Extra saves use fixed pages in each flash half and a 16-byte footer.
    for half in (0, HALF_SIZE):
        for ident, page in enumerate((36, 38, 40, 42), start=2):
            offset = half + page * 0x1000
            old = data[offset:offset + 7504]
            signature, counter, size, footer_id, checksum = struct.unpack_from(
                '<IIIHH', data, offset + 7504)
            if (signature != SIGNATURE or size != 7504 or footer_id != ident
                    or checksum != crc(data[offset:offset + 7518])):
                continue
            new = bytearray(7512)
            new[:132] = old[:132]
            new[136:236] = old[132:232]
            new[240:] = old[232:]
            # The native validator hashes 92 bytes of battleInfo, compared with
            # 88 on DS; the encrypted battleData itself has identical layout.
            struct.pack_into('<H', new, 232, crc(new[136:228]))
            footer = struct.pack('<IIIH', signature, counter, len(new), ident)
            record = new + footer
            record += struct.pack('<H', crc(record))
            output[offset:offset + len(record)] = record
            converted += 1
    return converted


def convert(data):
    data = normalize(data)
    if all(valid_blocks(data, PC_BLOCKS)):
        raise ValueError('This save already uses the current PC layout; copy it '
                         'to an unused save1.bin, save2.bin, or save3.bin slot.')
    pair = select_pair(valid_blocks(data, DS_BLOCKS))
    output = bytearray(data)  # Preserve Hall of Fame, frontier and extra saves.
    for half in (0, HALF_SIZE):
        # Rebuild both normal save copies from the most recent consistent pair.
        output[half:half + 0x20000] = b'\xff' * 0x20000
        offset = half
        for ident, (counter, block_counter, old_body) in enumerate(pair):
            body = convert_body(ident, old_body)
            block = body + FOOTER.pack(counter, block_counter, len(body) + 20,
                                       SIGNATURE, ident, crc(body))
            output[offset:offset + len(block)] = block
            offset += len(block)
    recordings = convert_recordings(data, output)
    select_pair(valid_blocks(output, PC_BLOCKS))
    return bytes(output), pair[0][0], recordings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='original DS .sav or DeSmuME .dsv')
    parser.add_argument('output', type=Path, help='new PC save file, e.g. save3.bin')
    args = parser.parse_args()
    try:
        result, counter, recordings = convert(args.input.read_bytes())
        # Exclusive creation prevents overwriting either the original or a slot.
        stream = args.output.open('xb')
        try:
            with stream:
                stream.write(result)
        except BaseException:
            args.output.unlink()
            raise
    except (OSError, ValueError) as error:
        print(f'Conversion failed: {error}', file=sys.stderr)
        return 1
    print(f'Created {args.output} (save counter {counter}; '
          f'{recordings} battle recording copies converted). Original unchanged.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
