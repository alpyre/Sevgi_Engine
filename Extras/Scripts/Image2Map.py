# Filename  : Image2Map.py
# Idea      : Original idea Georg Muntingh and Bjorn Lindeijer (version 1.2)
# Authors   : Chat-GPT (upgraded to Python 3.x using Chat-GPT with new features)
# Version   : 2.0
# Date      : June 25, 2026
# Copyright : Public Domain
# Requires  : Python 3.x
# Purpose   : Parses a map image and extracts a tileset from it
# Usage     : Image2Map.py <tileWidth> <tileHeight> <image.png> [similarity%]
#             [similarity%] helps discarding perceptually similar tiles
#                           0: default (removes only completely identical tiles)
#                           5: ideal (higher values discard more aggresively)
#                           NOTE: You will also be given an image-remapped.png
# Info      : https://github.com/mapeditor/tiled/wiki/Import-from-Image

import sys
import os
import numpy as np
from PIL import Image


# -----------------------------
# Perceptual Hash (aHash)
# -----------------------------
def average_hash(tile, hash_size=8):
    """Compute average hash (aHash) for a tile."""
    small = tile.resize((hash_size, hash_size), Image.Resampling.LANCZOS).convert("L")
    pixels = np.array(small, dtype=np.float32)

    avg = pixels.mean()
    diff = pixels > avg

    # Pack into integer
    return sum(1 << i for i, v in enumerate(diff.flatten()) if v)


def hamming_distance(h1, h2):
    """Hamming distance between two hashes."""
    return (h1 ^ h2).bit_count()


# -----------------------------
# Fast NumPy pixel difference
# -----------------------------
def tile_difference_percent_np(tile1_arr, tile2_arr):
    """Return % of different pixels using NumPy."""
    diff = np.count_nonzero(tile1_arr != tile2_arr)
    total = tile1_arr.size
    return (diff / total) * 100.0


# -----------------------------
# Tile splitting
# -----------------------------
def split_tiles(image, tile_w, tile_h):
    width, height = image.size
    tiles_x = width // tile_w
    tiles_y = height // tile_h

    tiles = []

    for y in range(tiles_y):
        for x in range(tiles_x):
            box = (
                x * tile_w,
                y * tile_h,
                (x + 1) * tile_w,
                (y + 1) * tile_h
            )
            tiles.append(image.crop(box))

    return tiles, tiles_x, tiles_y


# -----------------------------
# Tileset packing
# -----------------------------
def build_tileset(unique_tiles, tile_w, tile_h):
    count = len(unique_tiles)

    cols = int(np.ceil(np.sqrt(count)))
    rows = int(np.ceil(count / cols))

    new_img = Image.new("RGB", (cols * tile_w, rows * tile_h))

    for i, tile in enumerate(unique_tiles):
        x = (i % cols) * tile_w
        y = (i // cols) * tile_h
        new_img.paste(tile, (x, y))

    return new_img


# -----------------------------
# Rebuild image
# -----------------------------
def rebuild_image(tile_indices, unique_tiles, tiles_x, tiles_y, tile_w, tile_h):
    new_img = Image.new("RGB", (tiles_x * tile_w, tiles_y * tile_h))

    i = 0
    for y in range(tiles_y):
        for x in range(tiles_x):
            tile_idx = tile_indices[i]
            tile = unique_tiles[tile_idx]
            new_img.paste(tile, (x * tile_w, y * tile_h))
            i += 1

    return new_img


# -----------------------------
# Main processing
# -----------------------------
def process_image(input_file, tile_w, tile_h, similarity_threshold):
    print(f"Loading image: {input_file}")

    image = Image.open(input_file).convert("RGB")

    tiles, tiles_x, tiles_y = split_tiles(image, tile_w, tile_h)

    print(f"Total tiles: {len(tiles)}")

    unique_tiles = []
    unique_arrays = []
    unique_hashes = []

    tile_map_indices = []

    # Heuristic: hash tolerance derived from similarity %
    hash_threshold = int((similarity_threshold / 100.0) * 64)

    for i, tile in enumerate(tiles):
        tile_arr = np.array(tile)
        tile_hash = average_hash(tile)

        match_index = None

        for idx, ref_hash in enumerate(unique_hashes):
            # Fast reject using hash
            if hamming_distance(tile_hash, ref_hash) > hash_threshold:
                continue

            # Precise check using NumPy
            diff = tile_difference_percent_np(tile_arr, unique_arrays[idx])

            if diff <= similarity_threshold:
                match_index = idx
                break

        if match_index is not None:
            tile_map_indices.append(match_index)
        else:
            unique_tiles.append(tile)
            unique_arrays.append(tile_arr)
            unique_hashes.append(tile_hash)
            tile_map_indices.append(len(unique_tiles) - 1)

        if i % 100 == 0:
            print(f"Processed {i}/{len(tiles)} tiles...")

    print(f"Unique tiles: {len(unique_tiles)}")
    print(f"Removed similar tiles: {len(tiles) - len(unique_tiles)}")

    # Save tileset
    tileset_img = build_tileset(unique_tiles, tile_w, tile_h)
    tileset_file = os.path.splitext(input_file)[0] + "-tileset.png"
    tileset_img.save(tileset_file, "PNG")

    print(f"Saved tileset: {tileset_file}")

    # Save remapped image
    rebuilt = rebuild_image(tile_map_indices, unique_tiles,
                            tiles_x, tiles_y, tile_w, tile_h)

    rebuilt_file = os.path.splitext(input_file)[0] + "-remapped.png"
    rebuilt.save(rebuilt_file, "PNG")

    print(f"Saved remapped image: {rebuilt_file}")


# -----------------------------
# Entry point
# -----------------------------
if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage:    python Image2Map.py <tileWidth> <tileHeight> <image.png> [similarity%]")
        print("Examples: python Image2Map.py 8 8 Cave.png")
        print("          python Image2Map.py 16 16 Sewers.png 5")
        sys.exit(1)

    tile_w = int(sys.argv[1])
    tile_h = int(sys.argv[2])
    input_file = sys.argv[3]
    if len(sys.argv) >= 5:
        similarity_threshold = float(sys.argv[4])
    else:
        similarity_threshold = float(0)

    process_image(input_file, tile_w, tile_h, similarity_threshold)
