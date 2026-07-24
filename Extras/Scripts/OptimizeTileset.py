# Filename  : OptimizeTileset.py
# Idea      : İ. Alper Sönmez
# Authors   : Vibe coded using Chat-GPT
# Version   : 2.0
# Date      : July 24, 2026
# Copyright : Public Domain
# Requires  : Python 3.x
# Purpose   : Optimizes a tileset by removing duplicates and reordering
# Usage     : OptimizeTileset.py <tileWidth> <tileHeight> <image.png>
#             [--nozero]       Removes the empty tile from the beginning
#             [--nodups]       Removes all identical tiles
#             [--reorder]      Packs visually similar tiles together
#             [--similarity %] Removes perceptually similar tiles
#                              0: removes only identical (--nodups is faster)
#                              5: default (higher values match more aggresively)
# Info      : https://github.com/alpyre/Sevgi_Engine

import argparse
import numpy as np
from PIL import Image


def compute_ahash(tile, hash_size=8):
    """Compute average hash (aHash) for a tile."""
    img = Image.fromarray(tile).convert("L").resize((hash_size, hash_size), Image.Resampling.LANCZOS)
    pixels = np.array(img)
    avg = pixels.mean()
    return pixels > avg


def hash_distance(h1, h2):
    """Hamming distance between two hashes."""
    return np.count_nonzero(h1 != h2)


def is_empty(tile):
    """Check if tile is completely black."""
    return np.all(tile == 0)


def pixel_difference_percent(t1, t2):
    """Compute % of different pixels."""
    diff = np.any(t1 != t2, axis=-1)
    return (np.count_nonzero(diff) / diff.size) * 100.0


def extract_tiles(img_array, tw, th):
    """Split image into tiles."""
    h, w = img_array.shape[:2]
    tiles = []
    for y in range(0, h, th):
        for x in range(0, w, tw):
            tiles.append(img_array[y:y+th, x:x+tw])
    return tiles


def rebuild_tileset(tiles, tw, th, output_path):
    """Save tiles into a new image."""
    cols = int(np.ceil(np.sqrt(len(tiles))))
    rows = int(np.ceil(len(tiles) / cols))

    out = np.zeros((rows * th, cols * tw, 4), dtype=np.uint8)

    for i, tile in enumerate(tiles):
        y = (i // cols) * th
        x = (i % cols) * tw
        out[y:y+th, x:x+tw] = tile

    Image.fromarray(out).save(output_path)


def main():
    parser = argparse.ArgumentParser(description="Advanced TileSet Optimizer")
    parser.add_argument("tile_width", type=int)
    parser.add_argument("tile_height", type=int)
    parser.add_argument("image", help="Input tileset PNG")

    parser.add_argument("--nozero", action="store_true", help="Do not insert empty tile at start")
    parser.add_argument("--nodups", action="store_true", help="Remove exact duplicate tiles")
    parser.add_argument("--reorder", action="store_true", help="Group similar tiles together")
    parser.add_argument("--similarity", type=float, default=None,
                        help="Similarity threshold (percentage difference)")

    args = parser.parse_args()

    img = Image.open(args.image).convert("RGBA")
    img_array = np.array(img)

    tiles = extract_tiles(img_array, args.tile_width, args.tile_height)

    unique_tiles = []
    hashes = []
    raw_seen = set()

    for tile in tiles:
        tile_bytes = tile.tobytes()

        # Skip empty tiles (handled later)
        if is_empty(tile):
            continue

        # Remove exact duplicates
        if args.nodups:
            if tile_bytes in raw_seen:
                continue
            raw_seen.add(tile_bytes)

        # Similarity filtering
        if args.similarity is not None:
            is_similar = False
            for ut in unique_tiles:
                diff = pixel_difference_percent(tile, ut)
                if diff <= args.similarity:
                    is_similar = True
                    break
            if is_similar:
                continue

        unique_tiles.append(tile)
        hashes.append(compute_ahash(tile))

    # Reorder tiles by similarity
    if args.reorder and len(unique_tiles) > 1:
        ordered = [unique_tiles.pop(0)]
        ordered_hashes = [hashes.pop(0)]

        while unique_tiles:
            last_hash = ordered_hashes[-1]

            distances = [hash_distance(last_hash, h) for h in hashes]
            idx = int(np.argmin(distances))

            ordered.append(unique_tiles.pop(idx))
            ordered_hashes.append(hashes.pop(idx))

        unique_tiles = ordered

    # Insert empty tile at start unless disabled
    if not args.nozero:
        empty_tile = np.zeros((args.tile_height, args.tile_width, 4), dtype=np.uint8)
        unique_tiles.insert(0, empty_tile)

    output_path = args.image.replace(".png", "_cleaned.png")
    rebuild_tileset(unique_tiles, args.tile_width, args.tile_height, output_path)

    print(f"Saved cleaned tileset to: {output_path}")
    print(f"Total tiles: {len(unique_tiles)}")


if __name__ == "__main__":
    main()
