# Filename  : RepackSheet.py
# Idea      : İ. Alper Sönmez
# Authors   : Vibe coded using Chat-GPT
# Version   : 1.0
# Date      : June 30, 2026
# Copyright : Public Domain
# Requires  : Python 3.x
# Purpose   : Takes an irregular sprite sheet and places every image into
#             even sized and evenly spaced regular bounding boxes
# Usage     : RepackSheet.py input.png
#             [--grid] draws the bounding boxes as a one pixel thick grid
# Info      : https://github.com/alpyre/Sevgi_Engine

from PIL import Image
import sys
import os
import hashlib

def get_background_color(img):
    return img.getpixel((0, 0))

def invert_color(color):
    return tuple(255 - c for c in color[:3]) + ((color[3],) if len(color) == 4 else ())

def is_bg(pixel, bg):
    return pixel == bg

def find_empty_rows(img, bg):
    width, height = img.size
    pixels = img.load()
    return [y for y in range(height) if all(is_bg(pixels[x, y], bg) for x in range(width))]

def find_empty_cols(img, bg, y1, y2):
    width, _ = img.size
    pixels = img.load()
    return [x for x in range(width) if all(is_bg(pixels[x, y], bg) for y in range(y1, y2))]

def split_ranges(indices, max_val):
    # --- Convert list of separator indices into content ranges ---
    ranges = []
    start = 0
    indices_set = set(indices)

    for i in range(max_val):
        if i in indices_set:
            if start < i:
                ranges.append((start, i))
            start = i + 1

    if start < max_val:
        ranges.append((start, max_val))

    return ranges

def crop_sprite(img, x1, x2, y1, y2):
    return img.crop((x1, y1, x2, y2))

def hash_image(img):
    return hashlib.md5(img.tobytes()).hexdigest()

def main(input_path, draw_grid=False):
    img = Image.open(input_path).convert("RGBA")
    bg = get_background_color(img)
    grid_color = invert_color(bg)

    width, height = img.size

    # --- Find row separators ---
    empty_rows = find_empty_rows(img, bg)
    row_ranges = split_ranges(empty_rows, height)

    sprites_grid = []
    all_sprites = []
    unique_hashes = set()
    duplicates_removed = 0

    # --- Process each row ---
    for (y1, y2) in row_ranges:
        empty_cols = find_empty_cols(img, bg, y1, y2)
        col_ranges = split_ranges(empty_cols, width)

        row_sprites = []

        for (x1, x2) in col_ranges:
            sprite = crop_sprite(img, x1, x2, y1, y2)

            # --- Skip fully empty  ---
            if all(pixel == bg for pixel in sprite.get_flattened_data()):
                continue

            h = hash_image(sprite)

            if h in unique_hashes:
                duplicates_removed += 1
                continue

            unique_hashes.add(h)

            row_sprites.append(sprite)
            all_sprites.append(sprite)

        if row_sprites:
            sprites_grid.append(row_sprites)

    if not sprites_grid:
        print("No sprites detected.")
        return

    # --- Compute max dimensions ---
    max_w = max(s.width for s in all_sprites)
    max_h = max(s.height for s in all_sprites)

    rows = len(sprites_grid)
    cols = max(len(r) for r in sprites_grid)

    # --- Add grid spacing if enabled ---
    spacing = 1 if draw_grid else 0

    out_w = cols * max_w + (cols - 1) * spacing
    out_h = rows * max_h + (rows - 1) * spacing

    out_img = Image.new("RGBA", (out_w, out_h), bg)

    # --- Draw grid background first (if enabled) ---
    if draw_grid:
        pixels = out_img.load()
        for y in range(out_h):
            for x in range(out_w):
                if (x % (max_w + spacing) == max_w) or (y % (max_h + spacing) == max_h):
                    pixels[x, y] = grid_color

    # --- Paste sprites ---
    for row_idx, row in enumerate(sprites_grid):
        for col_idx, sprite in enumerate(row):
            x = col_idx * (max_w + spacing)
            y = row_idx * (max_h + spacing)
            out_img.paste(sprite, (x, y))

    # --- Output filename ---
    base, _ = os.path.splitext(input_path)
    output_path = f"{base}_repacked.png"

    out_img.save(output_path)

    print(f"Columns x Rows:     {cols} x {rows}")
    print(f"Bounding box size:  {max_w} x {max_h}")
    if draw_grid:
        print("Set spacings to:    1 x 1")
    else:
        print("Set spacings to:    0 x 0")
    print(f"Removed duplicates: {duplicates_removed}")
    print(f"Exported images:    {len(all_sprites)}")
    print(f"Output sheet:       {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python RepackSheet.py input.png [--grid]")
    else:
        input_file = sys.argv[1]
        grid_flag = "--grid" in sys.argv
        main(input_file, draw_grid=grid_flag)
