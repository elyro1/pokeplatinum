#!/usr/bin/env python3
"""Import enlarged trainer sheets into one binary MCSS bundle and local preview atlases.

Usage: import_bw_trainer_backs.py back_sprite res/bwbattle/trainers
The build uses only back_sprites.bin. Source sheets and PNG/JSON previews stay local.
"""

import argparse
import json
import struct
from pathlib import Path

from PIL import Image, ImageDraw


NAMES = ["player_male", "player_female", "rival", "trainer_cheryl",
         "trainer_riley", "trainer_marley", "trainer_buck", "trainer_mira"]

# Shoulder in the standing body's cropped pixels, then the cut joint in each loose arm (in cell order).
# Anchor the joint itself, rather than borrowing Nate/Rosa's offsets for differently proportioned artwork.
ARM_JOINTS = {
    "player_male": ((29, 45), [(12, 7), (30, 38), (32, 36)]),
    "player_female": ((36, 45), [(12, 7), (27, 9), (11, 38), (27, 31)]),
    "rival": ((28, 44), [(11, 7), (25, 36), (31, 33)]),
    "trainer_cheryl": ((30, 43), [(11, 7), (23, 32), (30, 32)]),
    "trainer_riley": ((33, 42), [(13, 8), (25, 34), (27, 34)]),
    "trainer_marley": ((36, 47), [(12, 7), (24, 37), (32, 35)]),
    "trainer_buck": ((29, 44), [(11, 7), (24, 35), (32, 34)]),
    "trainer_mira": ((37, 48), [(10, 6), (26, 35), (28, 35)]),
}

# These are pivots inside the resting sleeves, not the top edge of the shoulder.
# Account for the pixels above each joint so the sleeve starts at the shoulder cap,
# rather than projecting up beside the neck. Raised arms keep their own attachment.
REST_SHOULDERS = {
    "player_male": (30, 48),
    "player_female": (36, 48),
    "rival": (29, 46),
    "trainer_cheryl": (31, 45),
    "trainer_riley": (33, 46),
    "trainer_marley": (35, 50),
    "trainer_buck": (30, 47),
    "trainer_mira": (36, 53),
}


def parts_from_sheet(path, female):
    image = Image.open(path).convert("RGBA")
    has_alpha = image.getchannel("A").getextrema()[0] < 255
    # Leave room for the transparent male sheet's complete outlines beside its loose arm pieces.
    width = 312 if female else (252 if path.stem == "player_male" else 256)
    image = image.resize((width, round(image.height * width / image.width)), Image.Resampling.NEAREST)
    pixels = image.load()
    # Older opaque sheets use a sampled key. Transparent sheets use alpha exclusively, retaining black outlines
    # and every character color regardless of the RGB stored in their transparent background pixels.
    top = [pixels[x, 0] for x in range(image.width)]
    key = tuple(sorted(p[channel] for p in top)[len(top) // 2] for channel in range(3))
    foreground = set()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, alpha = pixels[x, y]
            # Allow compression noise around the key, while distinguishing pink from warm skin highlights.
            if has_alpha:
                # MCSS's indexed texture has binary transparency. Discard faint export noise at the edges;
                # actual sprite pixels in the supplied sheets have alpha near 255.
                background = alpha < 128
            else:
                background = max(abs(r - key[0]), abs(g - key[1]), abs(b - key[2])) <= 40 and max(
                    abs((r - g) - (key[0] - key[1])), abs((b - g) - (key[2] - key[1]))) <= 24
            if not background:
                foreground.add((x, y))
    parts = []
    while foreground:
        point = foreground.pop()
        pending, component = [point], [point]
        while pending:
            x, y = pending.pop()
            for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)):
                neighbour = (x + dx, y + dy)
                if neighbour in foreground:
                    foreground.remove(neighbour)
                    pending.append(neighbour)
                    component.append(neighbour)
        if len(component) < 30:
            continue
        left, top = min(x for x, y in component), min(y for x, y in component)
        right, bottom = max(x for x, y in component) + 1, max(y for x, y in component) + 1
        part = Image.new("RGBA", (right - left, bottom - top))
        # Copy this component only: neighbouring poses have interleaving bounding boxes.
        for x, y in component:
            part.putpixel((x - left, y - top), (*pixels[x, y][:3], 255))
        parts.append((left, top, part))
    return sorted(parts, key=lambda part: (part[0], part[1]))


def pack(parts):
    free = [(0, 0, 256, 128)]
    placed = {}
    for index in sorted(range(len(parts)), key=lambda i: parts[i].width * parts[i].height, reverse=True):
        width, height = parts[index].size
        choices = [(min(w - width, h - height), max(w - width, h - height), x, y)
                   for x, y, w, h in free if w >= width and h >= height]
        if not choices:
            raise ValueError("Trainer parts do not fit in a 256x128 texture")
        _, _, left, top = min(choices)
        placed[index] = (left, top)
        right, bottom = left + width, top + height
        split = []
        for x, y, w, h in free:
            if x >= right or x + w <= left or y >= bottom or y + h <= top:
                split.append((x, y, w, h))
                continue
            if x < left:
                split.append((x, y, left - x, h))
            if x + w > right:
                split.append((right, y, x + w - right, h))
            if y < top:
                split.append((x, y, w, top - y))
            if y + h > bottom:
                split.append((x, bottom, w, y + h - bottom))
        split = list(dict.fromkeys(split))
        free = [rect for i, rect in enumerate(split) if not any(
            i != j and a <= rect[0] and b <= rect[1] and a + c >= rect[0] + rect[2]
            and b + d >= rect[1] + rect[3] for j, (a, b, c, d) in enumerate(split))]
    return placed


def make_indexed(image):
    # These supplied sprites use more colors than the original BW trainers. Keep 255 opaque colors rather than
    # merging the small face/hand regions into clothing, hair or hat shades to meet a 16-color limit.
    colors = [p[:3] for p in image.get_flattened_data() if p[3]]
    strip = Image.new("RGB", (len(colors), 1))
    strip.putdata(colors)
    palette = strip.quantize(colors=255, method=Image.Quantize.MEDIANCUT).getpalette()[:765]
    reference = Image.new("P", (1, 1))
    reference.putpalette(palette + palette[:3])
    quantized = image.convert("RGB").quantize(palette=reference, dither=Image.Dither.NONE)
    result = Image.new("P", image.size)
    result.putpalette([0, 0, 0] + palette)
    result.putdata([(color % 255) + 1 if pixel[3] else 0
                    for color, pixel in zip(quantized.get_flattened_data(), image.get_flattened_data())])
    result.info["transparency"] = 0
    return result


def import_sheet(source, target, name):
    female = name == "player_female"
    components = parts_from_sheet(source / (name + ".png"), female)
    if name == "trainer_buck" and len(components) == 5:
        # The new sheet's first two pairs of trousers meet at the bottom. Split at the narrow gap between poses,
        # retaining the pixels on both sides instead of treating the two bodies as one animation cell.
        left, top, joined = components.pop(0)
        alpha = joined.getchannel("A")
        cut = min(range(joined.width * 2 // 5, joined.width * 3 // 5),
                  key=lambda x: sum(a > 0 for a in alpha.crop((x, 0, x + 1, joined.height)).tobytes()))
        for x0, x1 in ((0, cut), (cut, joined.width)):
            part = joined.crop((x0, 0, x1, joined.height))
            bounds = part.getbbox()
            components.append((left + x0 + bounds[0], top + bounds[1], part.crop(bounds)))
        components.sort(key=lambda part: (part[0], part[1]))
    if len(components) != (7 if female else 6):
        raise ValueError(f"{name}: unexpected component count {len(components)}")
    if female:
        # Rosa's cells: body, straight arm, upper sleeve, forearm, bent arm, wind-up, follow-through.
        order = [0, 6, 1, 4, 5, 2, 3]
        anchors = [(-25, 48), (-20, 14), (-28, 39), (-12, 29), (-27, 32), (-42, 52), (-51, 38)]
    else:
        # Nate's cells: body, straight arm, bent arm, raised arm, wind-up, follow-through.
        order = [0, 5, 3, 4, 1, 2]
        anchors = [(-26, 0), (-25, 13), (-32, 41), (-43, 34), (-25, 0), (-28, 0)]
    parts = [components[i][2] for i in order]
    if name == "trainer_mira":
        # Her standing pose still paints the resting right arm. Keep the sleeve, torso, skirt and leg,
        # clearing the dangling limb so it cannot remain beneath the separately animated right arm.
        ImageDraw.Draw(parts[0]).polygon(
            [(29, 56), (29, 69), (31, 74), (34, 81), (36, 86), (39, 93), (39, 104),
             (48, 109), (48, 56)], fill=(0, 0, 0, 0))
    if female:
        # This sheet retains Rosa's pale upper-sleeve piece. Match it to Dawn's coat using the supplied red arm,
        # rather than treating it as a scarf (the animation rotates it around the shoulder).
        coat = [p[:3] for p in parts[4].get_flattened_data()
                if p[3] and p[0] > 75 and p[0] > p[1] * 1.4 and p[0] > p[2] * 1.3]
        sample = Image.new("RGB", (len(coat), 1))
        sample.putdata(coat)
        reduced = sample.quantize(colors=3, method=Image.Quantize.MEDIANCUT).convert("RGB")
        shades = sorted(set(reduced.get_flattened_data()), key=lambda rgb: sum(rgb))
        sleeve = parts[2]
        sleeve.putdata([(*shades[0 if max(p[:3]) < 160 else 1 if max(p[:3]) < 195 else 2], p[3])
                        if p[3] else p for p in sleeve.get_flattened_data()])
    # Keep the three poses at BW's original hip baseline, allowing different character heights.
    for i, baseline in ((0, -56), (5 if female else 4, -63), (6 if female else 5, -58)):
        anchors[i] = (anchors[i][0], parts[i].height + baseline)
    shoulder, joints = ARM_JOINTS[name]
    if not parts[0].getpixel(shoulder)[3]:
        raise ValueError(f"{name}: shoulder attachment is outside the body artwork")
    # BW2's standing body node is (0, -56), or (-2, -56) for Rosa. Its cropped bottom is the hip baseline.
    shoulder = (anchors[0][0] + shoulder[0] - (2 if female else 0), shoulder[1] - parts[0].height)
    attachments = [(0, 0)] * len(parts)
    for i, (joint_x, joint_y) in enumerate(joints, start=1):
        if not parts[i].getpixel((joint_x, joint_y))[3]:
            raise ValueError(f"{name}: cell {i} joint is outside the arm artwork")
        anchors[i] = (-joint_x, joint_y)
        attachments[i] = shoulder
    rest_shoulder = REST_SHOULDERS[name]
    if not parts[0].getpixel(rest_shoulder)[3]:
        raise ValueError(f"{name}: resting shoulder is outside the body artwork")
    attachments[1] = (anchors[0][0] + rest_shoulder[0] - (2 if female else 0),
                      rest_shoulder[1] - parts[0].height)
    if female:
        # The forearm pivots at the elbow of the separate upper sleeve. Reflect its local endpoint in X,
        # just as MCSS reflects the sleeve, and keep the elbow buried within both pieces.
        attachments[3] = (shoulder[0] + joints[1][0] - 5, shoulder[1] + 10 - joints[1][1])
    positions = pack(parts)
    atlas = Image.new("RGBA", (256, 128))
    cells = []
    for i, part in enumerate(parts):
        s, t = positions[i]
        atlas.paste(part, (s, t))
        x, y = anchors[i]
        attach_x, attach_y = attachments[i]
        cells.append(dict(x=x, y=y, width=part.width, height=part.height, s=s, t=t,
                          attachX=attach_x, attachY=attach_y))
    cells.append(dict(x=256, y=-256, width=0, height=0, s=0, t=0, attachX=0, attachY=0))
    indexed = make_indexed(atlas)
    indexed.save(target / (name + "_back.png"), optimize=True)
    # BWTB v3 record: trainer ID, cell count, quads with joint attachments, RGB555 palette and palette indices.
    data = bytearray(struct.pack("<HH", NAMES.index(name) + 1, len(cells)))
    for cell in cells:
        data.extend(struct.pack("<hhHHHHhh", *(cell[key] for key in
                                              ("x", "y", "width", "height", "s", "t", "attachX", "attachY"))))
    palette = indexed.getpalette()
    colors = [(palette[i] >> 3) | ((palette[i + 1] >> 3) << 5) | ((palette[i + 2] >> 3) << 10)
              for i in range(0, 768, 3)]
    data.extend(struct.pack("<256H", *colors))
    data.extend(indexed.tobytes())
    entry = dict(name=name, base=1 if female else 0, image=name + "_back.png", cells=cells)
    return entry, data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    entries, records = zip(*(import_sheet(args.source, args.output, name) for name in NAMES))
    bundle = struct.pack("<4sHH", b"BWTB", 3, len(records)) + b"".join(records)
    (args.output / "back_sprites.bin").write_bytes(bundle)
    (args.output / "back_sprites.json").write_text(json.dumps(entries, indent=2) + "\n")


if __name__ == "__main__":
    main()
