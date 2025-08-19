#ifndef DISKIO_H
#define DISKIO_H

#define BM_TYPE_BITMAP      0x0
#define BM_TYPE_DISPLAYABLE BMF_DISPLAYABLE
#define BM_TYPE_INTERLEAVED BMF_INTERLEAVED
#define BM_TYPE_GAMEFONT    BMF_DISPLAYABLE
#define BM_TYPE_BOBSHEET    0x26

#ifndef Unity
#define Unity 0x10000UL
#endif

struct TileSet *loadTileSet(STRPTR file, struct BitMap *friend);
struct TileMap *loadTileMap(STRPTR file, UWORD scr_width, UWORD scr_height);
struct BitMap* loadILBMBitMap(STRPTR fileName, ULONG type, ULONG extra_width);
struct SfxStructure* PT_Load8SVX(STRPTR fileName);
VOID PT_FreeSFX(struct SfxStructure* sfx);
struct SpriteBank* loadSpriteBank(STRPTR fileName);
VOID freeSpriteBank(struct SpriteBank* bank);
struct BOBSheet* loadBOBSheet(STRPTR fileName);
VOID freeBOBSheet(struct BOBSheet* bs);

#endif /* DISKIO_H */
