#!/usr/bin/env python3
# make-icon.py: generate the Homebrew Channel icon for the Wii port of Fuse
#
# Usage: python3 data/wii/make-icon.py [output.png]
#   Requires Pillow. Writes data/wii/icon.png by default.
#
# The icon is 128x48 pixels (the size the Homebrew Channel expects). It is
# built from files already in the Fuse source tree:
# - the Fuse application icon (data/icons/128x128), a ZX Spectrum 48K
# - the widget UI font (ui/widget/fuse.font.sbf), for the "Fuse" title
# The result is committed as data/wii/icon.png, so building Fuse for the Wii
# does not need Pillow; run this script again only to change the design.

import os
import sys

from PIL import Image, ImageDraw

WIDTH, HEIGHT = 128, 48
# Vertical gradient, light enough at the top for the black Spectrum to show
BACKGROUND_TOP = (88, 88, 96)
BACKGROUND_BOTTOM = (16, 16, 20)
TEXT_COLOUR = (255, 255, 255, 255)
# The four stripes of the ZX Spectrum logo
STRIPES = [(216, 0, 0, 255), (255, 216, 0, 255), (0, 192, 0, 255),
           (0, 192, 216, 255)]

src_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
icon_path = os.path.join(src_dir, "data", "icons", "128x128",
                         "net.sourceforge.fuse_emulator.Fuse.png")
font_path = os.path.join(src_dir, "ui", "widget", "fuse.font.sbf")
output_path = (sys.argv[1] if len(sys.argv) > 1 else
               os.path.join(src_dir, "data", "wii", "icon.png"))


def read_sbf_font(path):
    """Read the glyphs of a Fuse .sbf font: {char: list of pixel rows}."""
    glyphs = {}
    code = None
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("[U+") and line.endswith("]"):
                code = chr(int(line[3:-1], 16))
                glyphs[code] = []
            elif code is not None and line and set(line) <= set(".0X"):
                glyphs[code].append(line.replace("X", "."))
            elif not line.startswith("#"):
                code = None
    return glyphs


def glyph_extent(rows):
    """Columns actually used by a glyph (first, last), ignoring blank rows."""
    cols = [x for row in rows for x, p in enumerate(row) if p == "0"]
    return (min(cols), max(cols)) if cols else (0, 2)


def draw_text(img, glyphs, text, x, y, scale, colour):
    """Draw proportionally spaced text; return the width drawn."""
    start = x
    for ch in text:
        rows = glyphs[ch]
        first, last = glyph_extent(rows)
        for gy, row in enumerate(rows):
            for gx, p in enumerate(row):
                if p == "0" and first <= gx <= last:
                    px = x + (gx - first) * scale
                    py = y + gy * scale
                    ImageDraw.Draw(img).rectangle(
                        [px, py, px + scale - 1, py + scale - 1], fill=colour)
        x += (last - first + 2) * scale
    return x - start - scale


def text_width(glyphs, text, scale):
    widths = [glyph_extent(glyphs[ch]) for ch in text]
    return sum((last - first + 2) * scale for first, last in widths) - scale


def main():
    img = Image.new("RGBA", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    for y in range(HEIGHT):
        t = y / (HEIGHT - 1)
        colour = tuple(round(a + (b - a) * t)
                       for a, b in zip(BACKGROUND_TOP, BACKGROUND_BOTTOM))
        draw.line([(0, y), (WIDTH - 1, y)], fill=colour + (255,))

    # Spectrum 48K on the left, cropped to its visible pixels
    spectrum = Image.open(icon_path).convert("RGBA")
    spectrum = spectrum.crop(spectrum.getbbox())
    target_h = HEIGHT - 8
    target_w = round(spectrum.width * target_h / spectrum.height)
    spectrum = spectrum.resize((target_w, target_h), Image.LANCZOS)
    img.alpha_composite(spectrum, (4, (HEIGHT - target_h) // 2))

    # "Fuse" title and the Spectrum stripes, centred in the space left
    glyphs = read_sbf_font(font_path)
    area_left = 4 + target_w + 4
    area_width = WIDTH - area_left - 4
    scale = 2
    title_w = text_width(glyphs, "Fuse", scale)
    title_x = area_left + (area_width - title_w) // 2
    draw_text(img, glyphs, "Fuse", title_x, 6, scale, TEXT_COLOUR)

    stripe_w, stripe_h, slant = 6, 12, 6
    stripes_w = len(STRIPES) * stripe_w + slant
    sx = area_left + (area_width - stripes_w) // 2
    sy = 28
    draw = ImageDraw.Draw(img)
    for i, colour in enumerate(STRIPES):
        x0 = sx + i * stripe_w
        draw.polygon([(x0 + slant, sy), (x0 + slant + stripe_w, sy),
                      (x0 + stripe_w, sy + stripe_h), (x0, sy + stripe_h)],
                     fill=colour)

    img.save(output_path)
    print(f"wrote {output_path} ({WIDTH}x{HEIGHT})")


if __name__ == "__main__":
    main()
