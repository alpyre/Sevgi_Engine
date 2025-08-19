///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>
#include <datatypes/soundclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <string.h>

#include "tilemap.h"
#include "tiles.h"
#include "gameobject.h"
#include "audio.h"
#include "display.h"
#include "diskio.h"
///

///locateStrInFile(fileHandle, str)
/******************************************************************************
 * A KMP algorithm that locates a string in a file without loading the file.  *
 * It returns TRUE if the string is found and the file seek position will be  *
 * at the byte just after the first instance of the found string. It begins   *
 * the search from the current seek position of the opened file.              *
 *****************************************************************************/
static BOOL locateStrInFile(BPTR fh, STRPTR str)
{
  BOOL found = FALSE;
  ULONG M = strlen(str);
  UBYTE* lps = AllocMem(M, MEMF_ANY | MEMF_CLEAR);

  if (lps) {
    LONG i;
    ULONG j = 0;
    UBYTE len = 0; // length of the previous longest prefix suffix
    UBYTE ch;

    i = 1;
    while (i < M) {
      if (str[i] == str[len]) {
        len++;
        lps[i] = len;
        i++;
      }
      else {
        if (len != 0) {
          len = lps[len - 1];
        }
        else {
          lps[i] = 0;
          i++;
        }
      }
    }

    i = Read(fh, &ch, 1);
    while (i > 0) {
      if (str[j] == ch) {
        j++;
      }

      if (j == M) {
        found = TRUE;
        break;
      }
      else {
        i = Read(fh, &ch, 1);
        if (i > 0 && str[j] != ch) {
          if (j != 0)
          j = lps[j - 1];
          else
          i = Read(fh, &ch, 1);
        }
      }
    }

    FreeMem(lps, M);
  }

  return found;
}
///

///loadTileSet(file, friend)
struct TileSet *loadTileSet(STRPTR file, struct BitMap *friend)
{
  struct TileSet *tileset = NULL;
  BPTR fh = Open(file, MODE_OLDFILE);

  if (fh)
  {
    struct FileInfoBlock fib;
    if ((ExamineFH(fh, &fib)))
    {
      ULONG size = fib.fib_Size / sizeof(struct Tile);
      if (!size || fib.fib_Size % sizeof(struct Tile)) {
        puts("Corrupt or incorrect tileset file!");
      }
      else
      {
        tileset = newTileSet(size, friend);
        if (tileset) {
          if (fib.fib_Size != Read(fh, tileset->tiles, fib.fib_Size)) {
            disposeTileSet(tileset); tileset = NULL;
            puts("Read error reading tileset file");
          }
        }
        else puts("Not enough memory for tileset");
      }
    }
    Close(fh);
  }
  else
    puts("Could not open tileset file");

  return tileset;
}
///
///loadTileMap(file)
/******************************************************************************
 * Loads the given map file (created by ConvertMap) into memory. It adds an   *
 * empty row at the top and an empty column at the left (because the scroll   *
 * routine in display.c does not display column 0 and row 0 of the map)!      *
 * So the width and height values on the returned map struct will be one      *
 * higher than the values in the read map file!                               *
 ******************************************************************************/
struct TileMap *loadTileMap(STRPTR file, UWORD scr_width, UWORD scr_height)
{
  UWORD width;
  UWORD height;

  struct TileMap *map = NULL;
  BPTR fh = Open(file, MODE_OLDFILE);
  if (fh) {
    struct FileInfoBlock fib;
    if (ExamineFH(fh, &fib)) {
      // Read width and height from the map file
      Read(fh, &width, 2);
      Read(fh, &height, 2);

      if (width) {
        if (height) {
          if (fib.fib_Size == width * height * sizeof(TILEID) + 4) {
            map = newTileMap(width + 1, height + 1, scr_width, scr_height);
            if (map) {
              ULONG r;
              memset(map->data, 0, map->width * sizeof(TILEID));

              for (r = 1; r < map->height; r++) {
                TILEID* d = map->data + (r * map->width);
                *d = 0;
                if (Read(fh, d + 1, width * sizeof(TILEID)) != width * sizeof(TILEID)) {
                  disposeTileMap(map); map = NULL;
                  puts("Read error during reading map file");
                  break;
                }
              }
            }
            else puts("Not enough memory for map");
          }
          else puts("Corrupt map file");
        }
        else puts("Invalid map height!");
      }
      else puts("Invalid map width!");
    }
    else puts("Couldn't read map file");
    Close(fh);
  }
  else puts ("Couldn't open map file");

  return map;
}
///
///createBOBMasks(bitmap)
/******************************************************************************
 * loadILBMBitMap() function allocates twice the height of the actual ILBM    *
 * to be able to store below the cookie-cut masks of the images when called   *
 * with BM_TYPE_BOBSHEET.                                                     *
 * This function fills those masks.                                           *
 ******************************************************************************/
VOID createBOBMasks(struct BitMap* bm)
{
  ULONG height = bm->Rows / 2; //bottom half of the bitmap is used for masks of the upper
  ULONG maskOffset = bm->BytesPerRow * height;
  UBYTE* rs = (UBYTE*)bm->Planes[0];       // row start
  UBYTE* rc;                               // read cursor per row
  UBYTE* rd;                               // read cursor per planeline
  UBYTE* wd;                               // write cursor per planeline
  ULONG bpr = bm->BytesPerRow / bm->Depth; // bytes per planeline
  ULONG longs = bpr / 4;
  ULONG word = bpr % 4;
  ULONG r, d, l;

  for (r = 0; r < height; r++, rs += bm->BytesPerRow) {
    for (l = 0, rc = rs; l < longs; l++, rc += 4) {
      ULONG mask_l = 0;
      for (d = 0, rd = rc; d < bm->Depth; d++, rd += bpr) {
        mask_l |= *(ULONG*)rd;
      }
      for (d = 0, wd = rc + maskOffset; d < bm->Depth; d++, wd += bpr) {
        *(ULONG*)wd = mask_l;
      }
    }
    if (word) {
      UWORD mask_w = 0;
      for (d = 0, rd = rc; d < bm->Depth; d++, rd += bpr) {
        mask_w |= *(UWORD*)rd;
      }
      for (d = 0, wd = rc + maskOffset; d < bm->Depth; d++, wd += bpr) {
        *(UWORD*)wd = mask_w;
      }
    }
  }
}
///
///loadILBMBitMap(fileName, type, extra_width)
struct BitMap* loadILBMBitMap(STRPTR fileName, ULONG type, ULONG extra_width)
{
  struct BitMapHeader bmhd;
  struct BitMap* bm = NULL;
  BPTR fh;

  fh = Open(fileName, MODE_OLDFILE);
  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "ILBM")) {
        if (locateStrInFile(fh, "BMHD")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct BitMapHeader) == Read(fh, &bmhd, sizeof(struct BitMapHeader))) {
            ULONG allocHeight   = bmhd.bmh_Height;
            ULONG allocFlags    = 0;
            ULONG allocNoCheck  = 1;
            ULONG allocMemCheck = MEMF_CHIP | MEMF_FAST;

            switch (type) {
              case BM_TYPE_BOBSHEET:
                if ((bmhd.bmh_Width % 16)) {
                  puts("ILBM width must be a multiple of 16!");
                  Close(fh);
                  return NULL;
                }
                allocHeight   = bmhd.bmh_Height * 2;
                allocFlags    = BMF_DISPLAYABLE | BMF_INTERLEAVED;
                allocNoCheck  = 0;
                allocMemCheck = MEMF_CHIP;
              break;
              case BM_TYPE_GAMEFONT:
              case BM_TYPE_BITMAP:
              default:
                if (type & BM_TYPE_DISPLAYABLE) {
                  allocFlags = BMF_DISPLAYABLE;
                  allocMemCheck = MEMF_CHIP;
                }
                if (type & BM_TYPE_INTERLEAVED) {
                  allocFlags |= BMF_INTERLEAVED;
                  allocNoCheck = 0;
                }
              break;
            }

            if (locateStrInFile(fh, "BODY")) {
              Seek(fh, 4, OFFSET_CURRENT);
              bm = allocBitMap(bmhd.bmh_Width + extra_width, allocHeight, bmhd.bmh_Depth, allocFlags, NULL);
              if (bm && (TypeOfMem(bm->Planes[0]) & allocMemCheck) && (allocNoCheck || (GetBitMapAttr(bm, BMA_FLAGS) & BMF_INTERLEAVED))) {
                if (bmhd.bmh_Compression) { // NOTE: consider possible compression methods
                  UBYTE* w;                         // write cursor
                  ULONG bpr = bmhd.bmh_Width / 8;   // bytes per row
                  ULONG row;
                  ULONG plane;
                  UBYTE b = 0;                      // read byte
                  UBYTE b2 = 0;

                  for (row = 0; row < bmhd.bmh_Height; row++) {
                    for (plane = 0; plane < bmhd.bmh_Depth; plane++) {
                      ULONG bytesWritten = 0;
                      w = (UBYTE*)bm->Planes[plane] + row * bm->BytesPerRow;
                      while (bytesWritten < bpr) {
                        //read a byte from body
                        Read(fh, &b, 1);
                        if (b < 128) {
                          Read(fh, w, b + 1);
                          w += b + 1;
                          bytesWritten += b + 1;
                        }
                        else if (b > 128) {
                          Read(fh, &b2, 1);
                          memset(w, b2, 257 - b);
                          w += (257 - b);
                          bytesWritten += (257 - b);
                        }
                      }
                    }
                    if (bmhd.bmh_Masking == 1) {
                      ULONG bytesSkipped = 0;
                      while (bytesSkipped < bpr) {
                        //read a byte from body
                        Read(fh, &b, 1);
                        if (b < 128) {
                          Seek(fh, b + 1, OFFSET_CURRENT);
                          bytesSkipped += b + 1;
                        }
                        else if (b > 128) {
                          Seek(fh, 1, OFFSET_CURRENT);
                          bytesSkipped += (257 - b);
                        }
                      }
                    }
                  }
                }
                else // uncompressed ilbm
                {
                  ULONG bpr = bmhd.bmh_Width / 8;   // bytes per row
                  ULONG row;
                  UBYTE *w = (UBYTE*)bm->Planes[0]; // write cursor
                  for (row = 0; row < bmhd.bmh_Height; row++) {
                    Read(fh, w, bmhd.bmh_Depth * bpr);
                    w += (bmhd.bmh_Depth * bpr);
                    if (bmhd.bmh_Masking == 1) {
                      Seek(fh, bpr, OFFSET_CURRENT);
                    }
                  }
                }

                if (type == BM_TYPE_BOBSHEET) {
                  createBOBMasks(bm);
                }
              }
              else if (bm) {
                FreeBitMap(bm);
                bm = NULL;
              }
            }
          }
        }
      }
    }
    Close(fh);
  }

  return bm;
}
///
///PT_Load8SVX(fileName)
struct SfxStructure* PT_Load8SVX(STRPTR fileName)
{
  struct SfxStructure* sfx = NULL;
  struct VoiceHeader vhdr;
  BPTR fh = Open(fileName, MODE_OLDFILE);

  if (fh) {
    if (locateStrInFile(fh, "FORM")) {
      if (locateStrInFile(fh, "8SVX")) {
        if (locateStrInFile(fh, "VHDR")) {
          Seek(fh, 4, OFFSET_CURRENT);
          if (sizeof(struct VoiceHeader) == Read(fh, &vhdr, sizeof(struct VoiceHeader))) {
            if (locateStrInFile(fh, "BODY")) {
              Seek(fh, 4, OFFSET_CURRENT);
              sfx = AllocMem(sizeof(struct SfxStructure), MEMF_ANY | MEMF_CLEAR);
              if (sfx) {
                // Load only the highest octave!
                UWORD samples = vhdr.vh_OneShotHiSamples + vhdr.vh_RepeatHiSamples;
                UWORD len = samples + 2;  // Add a zero word for idle looping
                if (samples % 2) len++;   // Add a pad byte if sample is odd
                sfx->sfx_len = len / 2;   // Set sample length in words
                // Calculate period from sampling frequency
                sfx->sfx_per = PAULA_CYCLES / vhdr.vh_SamplesPerSec;
                // Clamp period to hardware limits
                if (sfx->sfx_per < PAULA_MIN_PERIOD) sfx->sfx_per = PAULA_MIN_PERIOD;
                // Get sample playback volume from file
                if (vhdr.vh_Volume < 256)
                  sfx->sfx_vol = (vhdr.vh_Volume * 64) / 255;
                else
                  sfx->sfx_vol = (vhdr.vh_Volume * 64) / Unity;
                // Set default playback channel for this sample
                sfx->sfx_cha = SFX_DEFAULT_CHANNEL;
                sfx->sfx_pri = SFX_DEFAULT_PRIORITY;

                sfx->sfx_ptr = AllocMem(len, MEMF_CHIP);
                if (sfx->sfx_ptr) {
                  UBYTE *buffer = (UBYTE*)((ULONG)sfx->sfx_ptr + 2); // Set buffer to right after zero word
                  *(UWORD*)sfx->sfx_ptr = 0;                         // Clear the zero word

                  switch (vhdr.vh_Compression) {
                    case CMP_NONE:
                    {
                      Read(fh, buffer, samples);
                      if (samples % 2) buffer[samples] = 0;
                    }
                    break;
                    case CMP_FIBDELTA:
                    {
                      ULONG i;
                      BYTE codeToDelta[16] = {-34,-21,-13,-8,-5,-3,-2,-1,0,1,2,3,5,8,13,21};
                      BYTE x = 0;
                      BYTE d = 0;

                      Seek(fh, 1, OFFSET_CURRENT); // skip pad byte
                      Read(fh, &x, 1);             // read initial value

                      for (i = 0; i < samples; i++) {
                        if (i & 1) {
                          x += codeToDelta[d & 0xF];
                          buffer[i] = x;
                        }
                        else {
                          Read(fh, &d, 1);        // WARNING: No error checks here!
                          x += codeToDelta[d >> 4];
                          buffer[i] = x;
                        }
                      }
                      if (samples % 2) buffer[i] = 0; // Clear the pad byte for odd length samples
                    }
                    break;
                    default:
                      puts("Unknown compression type!");
                      PT_FreeSFX(sfx);
                      sfx = NULL;
                    break;
                  }
                }
                else {
                  puts("Not enough memory to load sample!");
                  FreeMem(sfx, sizeof(struct SfxStructure));
                  sfx = NULL;
                }
              }
            }
          }
        }
      }
    }

    Close(fh);
  }

  return sfx;
}
///
///PT_FreeSFX(sfx)
VOID PT_FreeSFX(struct SfxStructure* sfx)
{
  if (sfx) {
    if (sfx->sfx_ptr) {
      FreeMem(sfx->sfx_ptr, sfx->sfx_len * 2);
    }
    FreeMem(sfx, sizeof(struct SfxStructure));
  }
}
///

///checkLocalType(type)
BOOL checkLocalType(UBYTE type)
{
  #ifdef BIG_IMAGE_SIZES
    UBYTE local_type = 0x4;
  #else
    #ifdef SMALL_IMAGE_SIZES
      UBYTE local_type = 0x2;
    #else
      UBYTE local_type = 0x0;
    #endif
  #endif

  #ifdef SMALL_HITBOX_SIZES
    local_type |= 0x10;
  #endif

  return (BOOL)(local_type == type);
}
///
///loadSpriteBank(fileName)
struct SpriteBank* loadSpriteBank(STRPTR fileName)
{
  struct SpriteBank* bank = NULL;
  struct {
    UWORD offset;  // Byte offset from WordsOfSpriteData[0] to the the image
    UBYTE type;    // Defines the sprite usage of image
    UBYTE hsn;     // Sprites in this bank will be displayed with (starting from) this hardware sprite
    IMAGE_COMMON
  }table;

  BPTR fh = Open(fileName, MODE_OLDFILE);
  if (fh) {
    UBYTE id[8];
    UWORD num_images;
    UBYTE type;

    Read(fh, id, 7);
    if (strncmp(id, "SPRBANK", 7) == 0) {
      Read(fh, &type, 1);

      if (checkLocalType(type)) {
        Read(fh, &num_images, sizeof(UWORD));

        bank = AllocMem(sizeof(struct SpriteBank) + (num_images * sizeof(struct SpriteImage)), MEMF_ANY | MEMF_CLEAR);
        if (bank) {
          bank->num_images = num_images;
          bank->table = (struct SpriteTable*) AllocMem(sizeof(struct SpriteTable) * bank->num_images + sizeof(UWORD), MEMF_ANY);
          if (bank->table) {
            ULONG i;
            for (i = 0; i < bank->num_images; i++) {
              Read(fh, &table, sizeof(table));
              bank->table[i].offset = table.offset;
              bank->table[i].type   = table.type;
              bank->image[i].width  = table.width;
              bank->image[i].height = table.height;
              bank->image[i].h_offs = table.h_offs;
              bank->image[i].v_offs = table.v_offs;
              bank->image[i].image_num = i;
              bank->image[i].sprite_bank = bank;
              bank->image[i].hsn = table.hsn;
            }

            Read(fh, &bank->dataSize, sizeof(UWORD));
            bank->table[bank->num_images].offset = bank->dataSize;

            bank->data = (UBYTE*) AllocMem(bank->dataSize, MEMF_CHIP);
            if (bank->data) {
              Read(fh, bank->data, bank->dataSize);

              //Read hitboxes
              if (type & 0x08) {
                Read(fh, &bank->num_hitboxes, sizeof(UWORD));
                bank->hitboxes = AllocMem(sizeof(struct HitBox) * bank->num_hitboxes, MEMF_ANY);
                if (bank->hitboxes) {
                  ULONG i;

                  //set start hitbox on each image
                  for (i = 0; i < num_images; i++) {
                    UWORD index;
                    Read(fh, &index, sizeof(UWORD));
                    if (index == 0xFFFF) bank->image[i].hitbox = NULL;
                    else bank->image[i].hitbox = &bank->hitboxes[index];
                  }

                  //set linked hitboxes on each hitbox
                  for (i = 0; i < bank->num_hitboxes; i++) {
                    struct {
                      HITBOX_COMMON
                      UWORD next;
                    }hitbox;
                    Read(fh, &hitbox, sizeof(hitbox));

                    bank->hitboxes[i].x1 = hitbox.x1;
                    bank->hitboxes[i].y1 = hitbox.y1;
                    bank->hitboxes[i].x2 = hitbox.x2;
                    bank->hitboxes[i].y2 = hitbox.y2;
                    bank->hitboxes[i].next = hitbox.next == 0xFFFF ? NULL : &bank->hitboxes[hitbox.next];
                  }
                }
              }
              else {
                bank->hitboxes = NULL;
              }
            }
            else {
              FreeMem(bank->table, sizeof(struct SpriteTable) * bank->num_images);
              FreeMem(bank, sizeof(struct SpriteBank));
              bank = NULL;
              puts("Not enough chip memory for sprite data");
            }
          }
          else {
            FreeMem(bank, sizeof(struct SpriteBank));
            bank = NULL;
            puts("Not enough memory for sprite table");
          }
        }
        else
          puts("Not enough memory for sprite bank");
      }
      else printf("SpriteBank %s is not compatible with compiled image/hitbox sizes:\nPlease check BIG_IMAGE_SIZES, SMALL_IMAGE_SIZES and SMALL_HITBOX_SIZES defines\n", fileName);
    }
    else printf("File %s is not a sprite bank file!", fileName);

    Close(fh);
  }
  else printf("Could not open sprite bank file: %s", fileName);

  return bank;
}
///
///freeSpriteBank(bank)
VOID freeSpriteBank(struct SpriteBank* bank)
{
  if (bank) {
    if (bank->hitboxes) {
      FreeMem(bank->hitboxes, sizeof(struct HitBox) * bank->num_hitboxes);
    }
    FreeMem(bank->data, bank->dataSize);
    FreeMem(bank->table, sizeof(struct SpriteTable) * bank->num_images + sizeof(UWORD));
    FreeMem(bank, sizeof(struct SpriteBank) + (bank->num_images * sizeof(struct SpriteImage)));
  }
}
///

///loadBobSheet(fileName)
/******************************************************************************
 * Allocates a new struct BOBSheet and loads the given bob sheet file (.sht)  *
 * from disk into it by loading the ilbm file refered by the file with        *
 * BM_TYPE_BOBSHEET and creates all the BOBImage structures it defines.       *
 ******************************************************************************/
#define PATH_BUFFER_LENGTH 128

struct BOBSheet* loadBOBSheet(STRPTR fileName)
{
  struct BOBSheet* bs = NULL;
  BPTR fh;
  UBYTE path[PATH_BUFFER_LENGTH];
  ULONG path_len = (ULONG)PathPart(fileName) - (ULONG)fileName;
  struct {
    UBYTE type;
    UWORD num_images;
  }properties;

  strncpy(path, fileName, path_len);
  path[path_len] = 0;

  fh = Open(fileName, MODE_OLDFILE);
  if (fh) {
    UBYTE id[8];
    UBYTE sheet_file[32];
    struct BitMap* sheet_ilbm = NULL;

    Read(fh, id, 8);
    if (strncmp(id, "BOBSHEET", 8) == 0) {
      ULONG i = 0;
      Read(fh, &sheet_file[i], 1);
      while (sheet_file[i]) {
        Read(fh, &sheet_file[++i], 1);
      }
      if (i) {
        AddPart(path, sheet_file, PATH_BUFFER_LENGTH);
        sheet_ilbm = loadILBMBitMap(path, BM_TYPE_BOBSHEET, 0);
        if (sheet_ilbm) {
          Read(fh, &properties.type, sizeof(properties.type));
          Read(fh, &properties.num_images, sizeof(properties.num_images));

          if (checkLocalType(properties.type & 0xFE)) {
            bs = (struct BOBSheet*)AllocMem(sizeof(struct BOBSheet) + properties.num_images * sizeof(struct BOBImage), MEMF_ANY);
            if (bs) {
              bs->num_images = properties.num_images;
              bs->bitmap = sheet_ilbm;

              switch (properties.type & 0x01) {
                case BST_DISTRUBUTED:
                {
                  struct {
                    UBYTE width;
                    UBYTE height;
                    UBYTE columns;
                    UBYTE rows;
                  }props;
                  ULONG i;
                  UWORD bytesPerRow;

                  Read(fh, &props, sizeof(props));

                  bytesPerRow = (((props.width - 1) / 16) + 1) * 2;
                  for (i = 0; i < properties.num_images; i++) {
                    UWORD col = i % props.columns;
                    UWORD row = i / props.columns;

                    bs->image[i].width = props.width;
                    bs->image[i].height = props.height;
                    bs->image[i].bytesPerRow = bytesPerRow;
                    bs->image[i].bob_sheet = sheet_ilbm;
                    bs->image[i].pointer = sheet_ilbm->Planes[0] + (row * props.height * sheet_ilbm->BytesPerRow) + (col * bytesPerRow);
                    bs->image[i].mask = bs->image[i].pointer + sheet_ilbm->Rows * sheet_ilbm->BytesPerRow / 2;
                  }
                }
                break;
                case BST_IRREGULAR:
                {
                  struct {
                    IMAGE_COMMON
                    UBYTE word; // x coord of image in bob sheet / 16 (NOTE: Bob images MUST be in a WORD boundary)
                    UWORD row;  // y coord of image in bob sheet.
                  }props;
                  ULONG i;

                  for (i = 0; i < properties.num_images; i++) {
                    Read(fh, &props.width, sizeof(props.width) * 2);
                    Read(fh, &props.h_offs, sizeof(props.h_offs) * 2);
                    Read(fh, &props.word, sizeof(props.word));
                    Read(fh, &props.row, sizeof(props.row));
                    bs->image[i].width = props.width;
                    bs->image[i].height = props.height;
                    bs->image[i].h_offs = props.h_offs;
                    bs->image[i].v_offs = props.v_offs;
                    bs->image[i].bytesPerRow = (((props.width - 1) / 16) + 1) * 2;
                    bs->image[i].bob_sheet = sheet_ilbm;
                    bs->image[i].pointer = sheet_ilbm->Planes[0] + (props.row * sheet_ilbm->BytesPerRow) + (props.word * 2);
                    bs->image[i].mask = bs->image[i].pointer + sheet_ilbm->Rows * sheet_ilbm->BytesPerRow / 2;
                  }
                }
                break;
              }
              //Load hitboxes
              if (properties.type & 0x08) {
                Read(fh, &bs->num_hitboxes, sizeof(UWORD));
                bs->hitboxes = AllocMem(sizeof(struct HitBox) * bs->num_hitboxes, MEMF_ANY);
                if (bs->hitboxes) {
                  ULONG i;

                  //set start hitbox on each image
                  for (i = 0; i < properties.num_images; i++) {
                    UWORD index;
                    Read(fh, &index, sizeof(UWORD));
                    if (index == 0xFFFF) bs->image[i].hitbox = NULL;
                    else bs->image[i].hitbox = &bs->hitboxes[index];
                  }

                  //set linked hitboxes on each hitbox
                  for (i = 0; i < bs->num_hitboxes; i++) {
                    struct {
                      HITBOX_COMMON
                      UWORD next;
                    }hitbox;
                    Read(fh, &hitbox, sizeof(hitbox));

                    bs->hitboxes[i].x1 = hitbox.x1;
                    bs->hitboxes[i].y1 = hitbox.y1;
                    bs->hitboxes[i].x2 = hitbox.x2;
                    bs->hitboxes[i].y2 = hitbox.y2;
                    bs->hitboxes[i].next = hitbox.next == 0xFFFF ? NULL : &bs->hitboxes[hitbox.next];
                  }
                }
              }
              else {
                bs->hitboxes = NULL;
              }
            }
            else { puts("Not enough memory for BOB sheet!"); FreeBitMap(sheet_ilbm); }
          }
          else { printf("Bob sheet %s is not compatible with compiled image/hitbox sizes:\nPlease check BIG_IMAGE_SIZES, SMALL_IMAGE_SIZES and SMALL_HITBOX_SIZES defines\n", fileName); FreeBitMap(sheet_ilbm); }
        }
        else puts("Bob sheet ilbm could not be loaded!");
      }
      else puts("Invalid bob sheet ilbm filename!");
    }
    else puts("Invalid bob sheet file!");

    Close(fh);
  }
  else printf("Bob sheet file %s could not be opened!\n", fileName);

  return bs;
}
///
///freeBOBSheet(sheet)
/******************************************************************************
 * Frees a bob sheet loaded by loadBOBSheet().                                *
 ******************************************************************************/
VOID freeBOBSheet(struct BOBSheet* bs)
{
  if (bs) {
    if (bs->hitboxes) {
      FreeMem(bs->hitboxes, sizeof(struct HitBox) * bs->num_hitboxes);
    }
    if (bs->bitmap) {
      WaitBlit();
      FreeBitMap(bs->bitmap);
    }
    FreeMem(bs, sizeof(struct BOBSheet) + bs->num_images * sizeof(struct BOBImage));
  }
}
///
