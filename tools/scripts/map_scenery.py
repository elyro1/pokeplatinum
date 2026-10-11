"""Static land-model helpers for the guarded widescreen scenery generators."""

import struct

PARAM_COUNTS = {0: 0, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2,
                0x24: 1, 0x25: 1, 0x26: 1, 0x27: 1, 0x28: 1,
                0x40: 1, 0x41: 0}


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def signed(value, bits):
    return (value ^ (1 << (bits - 1))) - (1 << (bits - 1))


def dictionary(data, offset):
    entries = offset + u16(data, offset + 6)
    stride = u16(data, entries)
    names = entries + u16(data, entries + 2)
    return [(data[names + i * 16:names + (i + 1) * 16].split(b"\0")[0].decode(),
             entries + 4 + i * stride) for i in range(data[offset + 1])]


def polygons(display_list, scale):
    """Decode the commands used by the supported original static meshes."""
    offset = 0
    position = [0, 0, 0]
    uv = (0, 0)
    color, normal, mode = 0x7FFF, 0x0007FC00, None
    vertices, result = [], []
    while offset < len(display_list):
        commands = display_list[offset:offset + 4]
        offset += 4
        for command in commands:
            count = PARAM_COUNTS[command]
            args = struct.unpack_from("<" + "I" * count, display_list, offset)
            offset += count * 4
            if command == 0x20:
                color = args[0]
            elif command == 0x21:
                normal = args[0]
            elif command == 0x22:
                uv = (signed(args[0] & 0xFFFF, 16) / 16,
                      signed(args[0] >> 16, 16) / 16)
            elif command == 0x40:
                mode, vertices = args[0], []
            elif command == 0x41:
                mode = None
            elif command == 0x23:
                position = [signed(args[0] & 0xFFFF, 16) / 4096,
                            signed(args[0] >> 16, 16) / 4096,
                            signed(args[1] & 0xFFFF, 16) / 4096]
            elif command == 0x24:
                position = [signed((args[0] >> (i * 10)) & 1023, 10) / 64
                            for i in range(3)]
            elif command in (0x25, 0x26, 0x27):
                for i, dimension in enumerate({0x25: (0, 1), 0x26: (0, 2),
                                               0x27: (1, 2)}[command]):
                    position[dimension] = signed((args[0] >> (i * 16)) & 0xFFFF, 16) / 4096
            elif command == 0x28:
                position = [position[i] + signed((args[0] >> (i * 10)) & 1023, 10) / 4096
                            for i in range(3)]
            if 0x23 <= command <= 0x28:
                vertices.append((tuple(c * scale for c in position), uv, color, normal))
                n = len(vertices)
                if mode == 0 and n % 3 == 0:
                    result.append(vertices[-3:])
                elif mode == 1 and n % 4 == 0:
                    result.append(vertices[-4:])
                elif mode == 2 and n >= 3:
                    result.append(vertices[-3:] if n % 2 else
                                  [vertices[-2], vertices[-3], vertices[-1]])
                elif mode == 3 and n >= 4 and n % 2 == 0:
                    result.append([vertices[-4], vertices[-3], vertices[-1], vertices[-2]])
    return result


def pack_commands(commands):
    result = bytearray()
    for i in range(0, len(commands), 4):
        group = commands[i:i + 4]
        result.extend(bytes(c[0] for c in group).ljust(4, b"\0"))
        for _, params in group:
            result.extend(struct.pack("<" + "I" * len(params), *params))
    return result


def encode_quad(vertices, scale):
    commands = [(0x40, (1,))]
    for position, uv, color, normal in vertices:
        coords = [round(c / scale * 4096) for c in position]
        if not all(-32768 <= c <= 32767 for c in coords):
            raise ValueError("Extension exceeds VTX_16 range")
        tex = [round(c * 16) for c in uv]
        if not all(-32768 <= c <= 32767 for c in tex):
            raise ValueError("Extension exceeds texture-coordinate range")
        commands += [(0x20, (color,)), (0x21, (normal,)),
                     (0x22, ((tex[0] & 0xFFFF) | ((tex[1] & 0xFFFF) << 16),)),
                     (0x23, ((coords[0] & 0xFFFF) | ((coords[1] & 0xFFFF) << 16),
                             coords[2] & 0xFFFF))]
    commands.append((0x41, ()))
    return pack_commands(commands)



def read_land(source):
    sizes = list(struct.unpack_from("<4I", source))
    start = 16 + sizes[0] + sizes[1]
    model = source[start:start + sizes[2]]
    block = u32(model, 16)
    mdl = block + u32(model, dictionary(model, block + 8)[0][1])
    shape_table = mdl + u32(model, mdl + 12)
    scale = struct.unpack_from("<i", model, mdl + 28)[0] / 4096
    shapes = []
    for name, entry in dictionary(model, shape_table):
        header = shape_table + u32(model, entry)
        dl_start = header + u32(model, header + 8)
        dl = model[dl_start:dl_start + u32(model, header + 12)]
        shapes.append((name, header, dl_start, dl, polygons(dl, scale)))
    return model, block, mdl, scale, shapes


def repack_land(source, additions, bounds, model_limit=0xF000, encode_extra=None):
    model, block, mdl, scale, shapes = read_land(source)
    sizes = list(struct.unpack_from("<4I", source))
    start = 16 + sizes[0] + sizes[1]
    # Repack only the display-list tail; dictionaries/materials/nodes stay put.
    result = bytearray(model[:min(s[2] for s in shapes)])
    for (_, header, _, dl, _), extra in zip(shapes, additions):
        extended_dl = dl + (encode_extra(extra, scale) if encode_extra else
                            b"".join(encode_quad(q, scale) for q in extra))
        if extra:
            # Added commands use normals, colors and texture coordinates.
            struct.pack_into("<I", result, header + 4, u32(model, header + 4) | 7)
        struct.pack_into("<II", result, header + 8, len(result) - header, len(extended_dl))
        result.extend(extended_dl)
    quad_count = sum(map(len, additions))
    for offset, delta in ((36, quad_count * 4), (38, quad_count), (42, quad_count)):
        struct.pack_into("<H", result, mdl + offset, u16(model, mdl + offset) + delta)
    struct.pack_into("<I", result, 8, len(result))
    struct.pack_into("<I", result, block + 4, len(result) - block)
    struct.pack_into("<I", result, mdl, len(result) - mdl)
    struct.pack_into("<I", result, mdl + 16, len(result) - mdl)  # no envelope matrices
    box_scale = struct.unpack_from("<i", model, mdl + 56)[0] / 4096
    struct.pack_into("<6h", result, mdl + 44,
                     *(round(c / box_scale * 4096) for c in bounds))
    if len(result) > model_limit:
        raise ValueError("Extended model exceeds the engine's model buffer")
    sizes[2] = len(result)
    return (struct.pack("<4I", *sizes) + source[16:start] + result
            + source[start + u32(source, 8):]), quad_count

