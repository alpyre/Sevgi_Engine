# Filename  : MapWriter.py
# Idea      : Original idea Georg Muntingh and Bjorn Lindeijer (version 1.1)
# Authors   : Chat-GPT (upgraded to Python 3.x with vibe-coding)
# Version   : 2.0
# Date      : June 25, 2026
# Copyright : Public Domain
# Requires  : Python 3.x
# Purpose   : Creates a Tiled map file from am image of the map using a tile set
# Usage     : MapWriter.py <tileWidth> <tileHeight> <map image> <tileset image>
# Info      : https://github.com/mapeditor/tiled/wiki/Import-from-Image

import os
import sys
from PIL import Image
import struct
import base64

from xml.dom.minidom import Document


class Tileset:
    def __init__(self, tileImageFile, tileWidth, tileHeight):
        self.TileWidth = tileWidth
        self.TileHeight = tileHeight
        self.Filename = tileImageFile
        self.Name = os.path.splitext(tileImageFile)[0]
        self.List = []
        self.TileDict = {}
        self.readTiles()

    def readTiles(self):
        TileImage = Image.open(self.Filename).convert("RGB")
        TileIW, TileIH = TileImage.size

        TilesetW = TileIW // self.TileWidth
        TilesetH = TileIH // self.TileHeight

        for y in range(TilesetH):
            for x in range(TilesetW):
                box = (
                    self.TileWidth * x,
                    self.TileHeight * y,
                    self.TileWidth * (x + 1),
                    self.TileHeight * (y + 1),
                )

                tile = TileImage.crop(box)
                self.List.append(tile)

                key = tile.tobytes()
                if key not in self.TileDict:
                    self.TileDict[key] = len(self.List) - 1

    def findTile(self, tileImage):
        key = tileImage.tobytes()
        return self.TileDict.get(key, -1) + 1


class TileMap:
    def __init__(self, mapImageFile, tileSet, tileWidth, tileHeight):
        self.MapImageFile = mapImageFile
        self.TileWidth = tileWidth
        self.TileHeight = tileHeight
        self.TileSet = tileSet
        self.List = []
        self.readMap()

    def readMap(self):
        MapImage = Image.open(self.MapImageFile).convert("RGB")

        MapImageWidth, MapImageHeight = MapImage.size
        self.Width = MapImageWidth // self.TileWidth
        self.Height = MapImageHeight // self.TileHeight

        progress = -1

        for y in range(self.Height):
            for x in range(self.Width):
                box = (
                    self.TileWidth * x,
                    self.TileHeight * y,
                    self.TileWidth * (x + 1),
                    self.TileHeight * (y + 1),
                )

                tile = MapImage.crop(box)
                self.List.append(self.TileSet.findTile(tile))

                p = ((x + y * self.Width) * 100) // (self.Width * self.Height)
                if progress != p:
                    progress = p
                    self.printProgress(progress)

        self.printProgress(100)
        print()

    def printProgress(self, percentage):
        print(" " * 20, end="\r")
        print(f"{percentage:3d}% ", end="\r")
        sys.stdout.flush()

    def write(self, fileName):
        doc = Document()

        map_el = doc.createElement("map")
        map_el.setAttribute("version", "1.0")
        map_el.setAttribute("orientation", "orthogonal")
        map_el.setAttribute("width", str(self.Width))
        map_el.setAttribute("height", str(self.Height))
        map_el.setAttribute("tilewidth", str(self.TileWidth))
        map_el.setAttribute("tileheight", str(self.TileHeight))

        tileset = doc.createElement("tileset")
        tileset.setAttribute("name", self.TileSet.Name)
        tileset.setAttribute("firstgid", "1")
        tileset.setAttribute("tilewidth", str(self.TileSet.TileWidth))
        tileset.setAttribute("tileheight", str(self.TileSet.TileHeight))

        image = doc.createElement("image")
        image.setAttribute("source", self.TileSet.Filename)

        tileset.appendChild(image)
        map_el.appendChild(tileset)

        layer = doc.createElement("layer")
        layer.setAttribute("name", "Ground")
        layer.setAttribute("width", str(self.Width))
        layer.setAttribute("height", str(self.Height))

        data = doc.createElement("data")
        data.setAttribute("encoding", "base64")

        # Build binary tile data
        tile_data = bytearray()
        for tileId in self.List:
            tile_data += struct.pack("<I", tileId)

        encoded = base64.b64encode(tile_data).decode("ascii")
        data.appendChild(doc.createTextNode(encoded))

        layer.appendChild(data)
        map_el.appendChild(layer)
        doc.appendChild(map_el)

        with open(fileName, "w", encoding="utf-8") as f:
            f.write(doc.toprettyxml(indent=" "))


if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage:   python MapWriter.py <tileWidth> <tileHeight> <map image> <tileset image>")
        print("Example: python MapWriter.py 16 16 JansHouse.png JansHouse-Tileset.png")
        sys.exit(1)

    tileX, tileY = int(sys.argv[1]), int(sys.argv[2])
    mapImageFile, tileImageFile = sys.argv[3], sys.argv[4]

    m = TileMap(mapImageFile, Tileset(tileImageFile, tileX, tileY), tileX, tileY)
    m.write(os.path.splitext(mapImageFile)[0] + ".tmx")
