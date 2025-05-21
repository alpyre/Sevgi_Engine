///includes

//standard headers
#include <stdio.h>
#include <string.h>

//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <datatypes/pictureclass.h>

//Amiga protos
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "utility.h"
#include "image_spec.h"
///
///globals
extern APTR g_MemoryPool;
///
///protos
struct ILBMImage* newILBMImageFromBitmap(struct BitMap* bm);
///

///loadHitboxes(fh, sheet, type)
BOOL loadHitboxes(BPTR fh, struct Sheet* sheet, UBYTE type)
{
  BOOL ret_val = TRUE;
  UWORD num_hitboxes = 0;
  UWORD* index = NULL;
  ULONG i;

  if (type & 0x08) {
    index = (UWORD*)AllocPooled(g_MemoryPool, sizeof(UWORD) * sheet->num_images);
    if (index) {
      Read(fh, &num_hitboxes, sizeof(UWORD));
      Read(fh, index, sizeof(UWORD) * sheet->num_images);

      if (type & 0x18) {
        struct __attribute__((packed)) HitBox_small {
          BYTE x1, y1;
          BYTE x2, y2;
          UWORD next_hitbox;
        }*hitboxes = AllocPooled(g_MemoryPool, sizeof(struct HitBox_small) * num_hitboxes);

        if (hitboxes) {
          Read(fh, hitboxes, sizeof(struct HitBox_small) * num_hitboxes);

          for (i = 0; (i < sheet->num_images) && ret_val; i++) {
            if (index[i] != 0xFFFF) {
              struct HitBox_small* hb = &hitboxes[index[i]];
              do {
                struct HitBox* hb_new = newHitBox(hb->x1, hb->y1, hb->x2, hb->y2, 0, 0);
                if (hb_new) {
                  AddTail((struct List*)&sheet->spec[i].hitboxes, (struct Node*)hb_new);
                  sheet->num_hitboxes++;
                }
                else {
                  ret_val = FALSE;
                  break;
                }
                if (hb->next_hitbox != 0xFFFF) hb = &hitboxes[hb->next_hitbox];
                else hb = NULL;
              } while(hb);
            }
          }

          FreePooled(g_MemoryPool, hitboxes, sizeof(struct HitBox_small) * num_hitboxes);
        }
        else ret_val = FALSE;
      }
      else /*(type is big)*/ {
        struct __attribute__((packed)) HitBox_big {
          WORD x1, y1;
          WORD x2, y2;
          UWORD next_hitbox;
        }*hitboxes = AllocPooled(g_MemoryPool, sizeof(struct HitBox_big) * num_hitboxes);

        if (hitboxes) {
          Read(fh, hitboxes, sizeof(struct HitBox_big) * num_hitboxes);

          for (i = 0; (i < sheet->num_images) && ret_val; i++) {
            if (index[i] != 0xFFFF) {
              struct HitBox_big* hb = &hitboxes[index[i]];
              do {
                struct HitBox* hb_new = newHitBox(hb->x1, hb->y1, hb->x2, hb->y2, 0, 0);
                if (hb_new) {
                  AddTail((struct List*)&sheet->spec[i].hitboxes, (struct Node*)hb_new);
                  sheet->num_hitboxes++;
                }
                else {
                  ret_val = FALSE;
                  break;
                }
                if (hb->next_hitbox != 0xFFFF) hb = &hitboxes[hb->next_hitbox];
                else hb = NULL;
              } while(hb);
            }
          }

          FreePooled(g_MemoryPool, hitboxes, sizeof(struct HitBox_big) * num_hitboxes);
        }
        else ret_val = FALSE;
      }

      FreePooled(g_MemoryPool, index, sizeof(UWORD) * sheet->num_images);
    }
    else ret_val = FALSE;
  }

  return ret_val;
}
///
///loadBobSheet(fileName)
/******************************************************************************
 * Allocates a new struct Sheet and loads the given bob sheet file (.sht)     *
 * from disk into it by loading the ilbm file refered by the file as an       *
 * ILBMImage and creates all the ImageSpec structures it defines.             *
 ******************************************************************************/
#define PATH_BUFFER_LENGTH 128

struct Sheet* loadBOBSheet(STRPTR fileName)
{
  struct Sheet* bs = NULL;
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
    struct ILBMImage* sheet_ilbm = NULL;

    Read(fh, id, 8);
    if (strncmp(id, "BOBSHEET", 8) == 0) {
      ULONG i = 0;
      Read(fh, &sheet_file[i], 1);
      while (sheet_file[i]) {
        Read(fh, &sheet_file[++i], 1);
      }
      if (i) {
        AddPart(path, sheet_file, PATH_BUFFER_LENGTH);
        sheet_ilbm = loadILBMImage(path, BMF_INTERLEAVED);
        if (sheet_ilbm) {
          Read(fh, &properties.type, sizeof(properties.type));
          Read(fh, &properties.num_images, sizeof(properties.num_images));

          bs = (struct Sheet*)AllocPooled(g_MemoryPool, sizeof(struct Sheet) + properties.num_images * sizeof(struct ImageSpec));
          if (bs) {
            bs->sheet_file = makeString(fileName);
            bs->ilbm_file = makeString(sheet_file);
            bs->type = properties.type;
            bs->num_images = properties.num_images;
            bs->num_hitboxes = 0;
            bs->image = sheet_ilbm;

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

                Read(fh, &props, sizeof(props));

                for (i = 0; i < properties.num_images; i++) {
                  UWORD col = i % props.columns;
                  UWORD row = i / props.columns;

                  bs->spec[i].x = col * props.width;
                  bs->spec[i].y = row * props.height;
                  bs->spec[i].width = props.width;
                  bs->spec[i].height = props.height;
                  bs->spec[i].h_offs = 0;
                  bs->spec[i].v_offs = 0;
                  bs->spec[i].width = props.width;
                  NewList((struct List*)&bs->spec[i].hitboxes);
                }
              }
              break;
              case BST_IRREGULAR:
              {
                ULONG i;

                switch (properties.type & 0x06) {
                  case 0x00:
                  {
                    struct {
                      UBYTE width;
                      UBYTE height;
                      WORD h_offs;
                      WORD v_offs;
                      UBYTE word;
                      UWORD row;
                    }props;

                    for (i = 0; i < properties.num_images; i++) {
                      Read(fh, &props.width, sizeof(props.width) * 2);
                      Read(fh, &props.h_offs, sizeof(props.h_offs) * 2);
                      Read(fh, &props.word, sizeof(props.word));
                      Read(fh, &props.row, sizeof(props.row));
                      bs->spec[i].width = props.width;
                      bs->spec[i].height = props.height;
                      bs->spec[i].h_offs = props.h_offs;
                      bs->spec[i].v_offs = props.v_offs;
                      bs->spec[i].x = props.word * 16;
                      bs->spec[i].y = props.row;
                      NewList((struct List*)&bs->spec[i].hitboxes);
                    }
                  }
                  break;
                  case 0x02:
                  {
                    struct {
                      UBYTE width;
                      UBYTE height;
                      BYTE h_offs;
                      BYTE v_offs;
                      UBYTE word;
                      UWORD row;
                    }props;

                    for (i = 0; i < properties.num_images; i++) {
                      Read(fh, &props.width, sizeof(props.width) * 2);
                      Read(fh, &props.h_offs, sizeof(props.h_offs) * 2);
                      Read(fh, &props.word, sizeof(props.word));
                      Read(fh, &props.row, sizeof(props.row));
                      bs->spec[i].width = props.width;
                      bs->spec[i].height = props.height;
                      bs->spec[i].h_offs = props.h_offs;
                      bs->spec[i].v_offs = props.v_offs;
                      bs->spec[i].x = props.word * 16;
                      bs->spec[i].y = props.row;
                      NewList((struct List*)&bs->spec[i].hitboxes);
                    }
                  }
                  break;
                  case 0x04:
                  {
                    struct {
                      UWORD width;
                      UWORD height;
                      WORD h_offs;
                      WORD v_offs;
                      UBYTE word;
                      UWORD row;
                    }props;

                    for (i = 0; i < properties.num_images; i++) {
                      Read(fh, &props.width, sizeof(props.width) * 2);
                      Read(fh, &props.h_offs, sizeof(props.h_offs) * 2);
                      Read(fh, &props.word, sizeof(props.word));
                      Read(fh, &props.row, sizeof(props.row));
                      bs->spec[i].width = props.width;
                      bs->spec[i].height = props.height;
                      bs->spec[i].h_offs = props.h_offs;
                      bs->spec[i].v_offs = props.v_offs;
                      bs->spec[i].x = props.word * 16;
                      bs->spec[i].y = props.row;
                      NewList((struct List*)&bs->spec[i].hitboxes);
                    }
                  }
                  break;
                }
              }
              break;
            }

            loadHitboxes(fh, bs, bs->type);

          }// TODO: We have to change these err. messages into requesters!
          else { puts("Not enough memory for BOB sheet!"); freeILBMImage(sheet_ilbm); }
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
///loadSpriteBank(fileName)
/******************************************************************************
 * Allocates a new struct Sheet and loads the given sprite bank file (.spr)   *
 * from disk into it by creating a temporary sheet bitmap and reading the     *
 * sprite WORDs into it. It also loads a copy of the file into ram to be used.*
 ******************************************************************************/
ULONG getSpriteBankFileSize(STRPTR fileName)
{
  ULONG size = 0;
  BPTR fh = Open(fileName, MODE_OLDFILE);

  if (fh) {
    UBYTE type = 0;
    UWORD num_images = 0;
    UWORD data_size = 0;
    ULONG table_size = 0;

    Seek(fh, 7, OFFSET_BEGINNING);
    Read(fh, &type, 1);
    Read(fh, &num_images, 2);

    switch (type & 0x06) {
      case 0x02:
        table_size = 8;
      break;
      case 0x04:
        table_size = 12;
      break;
      default:
        table_size = 10;
    }

    Seek(fh, num_images * table_size, OFFSET_CURRENT);
    Read(fh, &data_size, 2);

    size = /*Header and type:*/ 8 +
           /*num_images     :*/ 2 +
           /*table          :*/ num_images * table_size +
           /*data_size      :*/ 2 +
           /*data           :*/ data_size;

    Close(fh);
  }

  return size;
}

struct Sheet* loadSpriteBank(STRPTR fileName)
{
  struct Sheet* sheet = NULL;
  BPTR fh;
  APTR spr_bank;
  ULONG file_size = getSpriteBankFileSize(fileName);

  if (file_size) {
    spr_bank = AllocPooled(g_MemoryPool, file_size);
    if (spr_bank) {
      fh = Open(fileName, MODE_OLDFILE);

      if (fh) {
        UWORD num_images;
        UBYTE type;

        Read(fh, spr_bank, file_size);

        type = ((UBYTE*)spr_bank)[7];
        num_images = ((UWORD*)spr_bank)[4];

        sheet = (struct Sheet*)AllocPooled(g_MemoryPool, sizeof(struct Sheet) + num_images * sizeof(struct ImageSpec));
        if (sheet) {
          sheet->sheet_file = makeString(fileName);
          sheet->ilbm_file = NULL;
          sheet->type = type;
          sheet->num_images = num_images;
          sheet->num_hitboxes = 0;
          sheet->sprite_bank = spr_bank;
          sheet->sprite_bank_size = file_size;

          //TODO: Data size optionality caused three times repeated code.
          //      Refactoring would be nice!
          switch (type & 0x06) {
            case 0x02:
            {
              struct Table {
                UWORD offset;
                UBYTE type;
                UBYTE hsn;
                UBYTE width;
                UBYTE height;
                BYTE  h_offs;
                BYTE  v_offs;
              } *table = (struct Table*)&((UBYTE*)spr_bank)[10];
              UBYTE* data = (UBYTE*)table + sizeof(struct Table) * num_images + sizeof(UWORD);
              struct BitMap* bm;
              ULONG i;
              ULONG max_width = 0;
              ULONG bitmap_height = 0;
              ULONG depth = 2;

              // Traverse the bank for image sizes and determine the size
              // for the sheet image!
              for (i = 0; i < num_images; i++) {
                if (table[i].type & 0x10) {
                  depth = 4;
                }
                if (table[i].width > max_width) {
                  max_width = table[i].width;
                }
                bitmap_height += table[i].height;
              }

              bm = AllocBitMap(max_width, bitmap_height, depth, BMF_INTERLEAVED, NULL);
              if (bm) {
                ULONG current_y = 0;
                sheet->image = newILBMImageFromBitmap(bm);

                if (sheet->image) {
                  for (i = 0; i < num_images; i++) {
                    ULONG sfmtype = table[i].type & 0x60;
                    ULONG sfmode = 1;
                    ULONG sfsize = 2;
                    ULONG num_sprites = table[i].type & 0x0F;
                    ULONG ssize = (table[i + 1].offset - table[i].offset) / num_sprites;
                    UBYTE* read_start;
                    UBYTE* read;
                    UBYTE* write;

                    switch (sfmtype) {
                      case 0x00:
                        sfmode = 1;
                        sfsize = 2;
                      break;
                      case 0x20:
                        sfmode = 2;
                        sfsize = 4;
                      break;
                      case 0x40:
                        sfmode = 4;
                        sfsize = 8;
                      break;
                    }

                    read_start = data + table[i].offset + 2 * sfsize;

                    if (table[i].type & 0x10) {
                      ULONG current_x = 0;

                      while (num_sprites) {
                        ULONG d, r;
                        for (d = 0; d < 4; d++) {
                          write = bm->Planes[d] + current_y * bm->BytesPerRow + current_x;
                          read = read_start + (d % 2) * sfsize;
                          for (r = 0; r < table[i].height; r++) {
                            memcpy(write, read, sfsize);
                            write += bm->BytesPerRow;
                            read += 2 * sfsize;
                          }
                          if (d % 2) {
                            read_start += ssize;
                            num_sprites--;
                          }
                        }
                        current_x += sfsize;
                      }
                    }
                    else {
                      ULONG current_x = 0;

                      while (num_sprites) {
                        ULONG d, r;
                        for (d = 0; d < 2; d++) {
                          write = bm->Planes[d] + current_y * bm->BytesPerRow + current_x;
                          read = read_start + d * sfsize;
                          for (r = 0; r < table[i].height; r++) {
                            memcpy(write, read, 2 * sfmode);
                            write += bm->BytesPerRow;
                            read += 2 * sfsize;
                          }
                        }
                        current_x += sfsize;
                        read_start += ssize;
                        num_sprites--;
                      }
                    }

                    sheet->spec[i].x = 0;
                    sheet->spec[i].y = current_y;
                    sheet->spec[i].width = table[i].width;
                    sheet->spec[i].height = table[i].height;
                    sheet->spec[i].h_offs = table[i].h_offs;
                    sheet->spec[i].v_offs = table[i].v_offs;
                    NewList((struct List*)&sheet->spec[i].hitboxes);

                    current_y += table[i].height;
                  }
                }
                else {
                  puts("Not enough memory!");
                  freeSheet(sheet); sheet = NULL;
                }
              }
              else {
                puts("Not enough memory!");
                freeSheet(sheet); sheet = NULL;
              }
            }
            break;
            case 0x04:
            {
              struct Table {
                UWORD offset;
                UBYTE type;
                UBYTE hsn;
                UWORD width;
                UWORD height;
                WORD  h_offs;
                WORD  v_offs;
              } *table = (struct Table*)&((UBYTE*)spr_bank)[10];
              UBYTE* data = (UBYTE*)table + sizeof(struct Table) * num_images + sizeof(UWORD);
              struct BitMap* bm;
              ULONG i;
              ULONG max_width = 0;
              ULONG bitmap_height = 0;
              ULONG depth = 2;

              // Traverse the bank for image sizes and determine the size
              // for the sheet image!
              for (i = 0; i < num_images; i++) {
                if (table[i].type & 0x10) {
                  depth = 4;
                }
                if (table[i].width > max_width) {
                  max_width = table[i].width;
                }
                bitmap_height += table[i].height;
              }

              bm = AllocBitMap(max_width, bitmap_height, depth, BMF_INTERLEAVED, NULL);
              if (bm) {
                ULONG current_y = 0;
                sheet->image = newILBMImageFromBitmap(bm);

                if(sheet->image) {
                  for (i = 0; i < num_images; i++) {
                    ULONG sfmtype = table[i].type & 0x60;
                    ULONG sfmode = 1;
                    ULONG sfsize = 2;
                    ULONG num_sprites = table[i].type & 0x0F;
                    ULONG ssize = (table[i + 1].offset - table[i].offset) / num_sprites;
                    UBYTE* read_start;
                    UBYTE* read;
                    UBYTE* write;

                    switch (sfmtype) {
                      case 0x00:
                        sfmode = 1;
                        sfsize = 2;
                      break;
                      case 0x20:
                        sfmode = 2;
                        sfsize = 4;
                      break;
                      case 0x40:
                        sfmode = 4;
                        sfsize = 8;
                      break;
                    }

                    read_start = data + table[i].offset + 2 * sfsize;

                    if (table[i].type & 0x10) {
                      ULONG current_x = 0;

                      while (num_sprites) {
                        ULONG d, r;
                        for (d = 0; d < 4; d++) {
                          write = bm->Planes[d] + current_y * bm->BytesPerRow + current_x;
                          read = read_start + (d % 2) * sfsize;
                          for (r = 0; r < table[i].height; r++) {
                            memcpy(write, read, sfsize);
                            write += bm->BytesPerRow;
                            read += 2 * sfsize;
                          }
                          if (d % 2) {
                            read_start += ssize;
                            num_sprites--;
                          }
                        }
                        current_x += sfsize;
                      }
                    }
                    else {
                      ULONG current_x = 0;

                      while (num_sprites) {
                        ULONG d, r;
                        for (d = 0; d < 2; d++) {
                          write = bm->Planes[d] + current_y * bm->BytesPerRow + current_x;
                          read = read_start + d * sfsize;
                          for (r = 0; r < table[i].height; r++) {
                            memcpy(write, read, 2 * sfmode);
                            write += bm->BytesPerRow;
                            read += 2 * sfsize;
                          }
                        }
                        current_x += sfsize;
                        read_start += ssize;
                        num_sprites--;
                      }
                    }

                    sheet->spec[i].x = 0;
                    sheet->spec[i].y = current_y;
                    sheet->spec[i].width = table[i].width;
                    sheet->spec[i].height = table[i].height;
                    sheet->spec[i].h_offs = table[i].h_offs;
                    sheet->spec[i].v_offs = table[i].v_offs;
                    NewList((struct List*)&sheet->spec[i].hitboxes);

                    current_y += table[i].height;
                  }
                }
                else {
                  puts("Not enough memory!");
                  freeSheet(sheet); sheet = NULL;
                }
              }
              else {
                puts("Not enough memory!");
                freeSheet(sheet); sheet = NULL;
              }
            }
            break;
            default:
            {
              struct Table {
                UWORD offset;
                UBYTE type;
                UBYTE hsn;
                UBYTE width;
                UBYTE height;
                WORD h_offs;
                WORD v_offs;
              } *table = (struct Table*)&((UBYTE*)spr_bank)[10];
              UBYTE* data = (UBYTE*)table + sizeof(struct Table) * num_images + sizeof(UWORD);
              struct BitMap* bm;
              ULONG i;
              ULONG max_width = 0;
              ULONG bitmap_height = 0;
              ULONG depth = 2;

              // Traverse the bank for image sizes and determine the size
              // for the sheet image!
              for (i = 0; i < num_images; i++) {
                if (table[i].type & 0x10) {
                  depth = 4;
                }
                if (table[i].width > max_width) {
                  max_width = table[i].width;
                }
                bitmap_height += table[i].height;
              }

              bm = AllocBitMap(max_width, bitmap_height, depth, BMF_INTERLEAVED, NULL);
              if (bm) {
                ULONG current_y = 0;
                sheet->image = newILBMImageFromBitmap(bm);

                if (sheet->image) {
                  for (i = 0; i < num_images; i++) {
                    ULONG sfmtype = table[i].type & 0x60;
                    ULONG sfmode = 1;
                    ULONG sfsize = 2;
                    ULONG num_sprites = table[i].type & 0x0F;
                    ULONG ssize = (table[i + 1].offset - table[i].offset) / num_sprites;
                    UBYTE* read_start;
                    UBYTE* read;
                    UBYTE* write;

                    switch (sfmtype) {
                      case 0x00:
                        sfmode = 1;
                        sfsize = 2;
                      break;
                      case 0x20:
                        sfmode = 2;
                        sfsize = 4;
                      break;
                      case 0x40:
                        sfmode = 4;
                        sfsize = 8;
                      break;
                    }

                    read_start = data + table[i].offset + 2 * sfsize;

                    if (table[i].type & 0x10) {
                      ULONG current_x = 0;

                      while (num_sprites) {
                        ULONG d, r;
                        for (d = 0; d < 4; d++) {
                          write = bm->Planes[d] + current_y * bm->BytesPerRow + current_x;
                          read = read_start + (d % 2) * sfsize;
                          for (r = 0; r < table[i].height; r++) {
                            memcpy(write, read, sfsize);
                            write += bm->BytesPerRow;
                            read += 2 * sfsize;
                          }
                          if (d % 2) {
                            read_start += ssize;
                            num_sprites--;
                          }
                        }
                        current_x += sfsize;
                      }
                    }
                    else {
                      ULONG current_x = 0;

                      while (num_sprites) {
                        ULONG d, r;
                        for (d = 0; d < 2; d++) {
                          write = bm->Planes[d] + current_y * bm->BytesPerRow + current_x;
                          read = read_start + d * sfsize;
                          for (r = 0; r < table[i].height; r++) {
                            memcpy(write, read, 2 * sfmode);
                            write += bm->BytesPerRow;
                            read += 2 * sfsize;
                          }
                        }
                        current_x += sfsize;
                        read_start += ssize;
                        num_sprites--;
                      }
                    }

                    sheet->spec[i].x = 0;
                    sheet->spec[i].y = current_y;
                    sheet->spec[i].width = table[i].width;
                    sheet->spec[i].height = table[i].height;
                    sheet->spec[i].h_offs = table[i].h_offs;
                    sheet->spec[i].v_offs = table[i].v_offs;
                    NewList((struct List*)&sheet->spec[i].hitboxes);

                    current_y += table[i].height;
                  }
                }
                else {
                  puts("Not enough memory!");
                  freeSheet(sheet); sheet = NULL;
                }
              }
              else {
                puts("Not enough memory!");
                freeSheet(sheet); sheet = NULL;
              }
            }
          }

          loadHitboxes(fh, sheet, sheet->type);
        }
        else {
          puts("Not enough memory!");
          FreePooled(g_MemoryPool, spr_bank, file_size);
        }

        Close(fh);
      }
      else puts("Cannot open file!");
    }
    else puts("Not enough memory!");
  }
  else puts("Invalid file size!");

  return sheet;
}
///
///freeSheet(sheet)
/******************************************************************************
 * Frees a bob sheet loaded by loadBOBSheet().                                *
 ******************************************************************************/
VOID freeSheet(struct Sheet* sheet)
{
  ULONG i;

  if (sheet) {
    if (sheet->image) {
      for (i = 0; i < sheet->num_images; i++) {
        struct HitBox* hb, *hb_next;
        for (hb = (struct HitBox*) sheet->spec[i].hitboxes.mlh_Head; (hb_next = (struct HitBox*)hb->node.mln_Succ); hb = hb_next) {
          deleteHitBox(hb);
        }
      }

      freeILBMImage(sheet->image);
    }
    if (sheet->sprite_bank) {
      FreePooled(g_MemoryPool, sheet->sprite_bank, sheet->sprite_bank_size);
    }
    freeString(sheet->sheet_file);
    freeString(sheet->ilbm_file);

    FreePooled(g_MemoryPool, sheet, sizeof(struct Sheet) + sheet->num_images * sizeof(struct ImageSpec));
  }
}
///

///newILBMImageFromBitmap(bitmap)
/******************************************************************************
 * To be used with loadSpriteBank()                                           *
 ******************************************************************************/
struct ILBMImage* newILBMImageFromBitmap(struct BitMap* bm)
{
  struct {
    UBYTE R, G, B;
  }def_cmap[16] = {{204, 204, 204},
                   {  0,   0,   0},
                   {255, 255, 255},
                   {136, 136, 136},
                   {170,  34,   0},
                   {221,  85,   0},
                   {238, 187,   0},
                   {  0, 136,   0},
                   {102, 187,   0},
                   { 17, 119,   0},
                   { 85, 187, 204},
                   { 51, 102, 119},
                   {  0, 119, 187},
                   {  0,  68, 136},
                   {238, 136, 136},
                   {187,  85,  85}};
  ULONG depth = GetBitMapAttr(bm, BMA_DEPTH);
  ULONG width = GetBitMapAttr(bm, BMA_WIDTH);
  ULONG height = GetBitMapAttr(bm, BMA_HEIGHT);
  ULONG num_colors = 1 << depth;
  struct ILBMImage* img = AllocPooled(g_MemoryPool, sizeof(struct ILBMImage) + num_colors * 3);
  if (img) {
    img->width = width;
    img->height = height;
    img->num_colors = num_colors;
    img->bitmap = bm;
    memcpy(img->cmap, def_cmap, num_colors * 3);
  }
  else {
    FreeBitMap(bm);
  }

  return img;
}
///

///newHitBox(x1, y1, x2, y2, index, next)
struct HitBox* newHitBox(WORD x1, WORD y1, WORD x2, WORD y2, UWORD index, UWORD next)
{
  struct HitBox* hb;

  hb = AllocPooled(g_MemoryPool, sizeof(struct HitBox));

  if (hb) {
    hb->x1 = x1;
    hb->y1 = y1;
    hb->x2 = x2;
    hb->y2 = y2;
    hb->index = index;
    hb->next = next;
  }

  return hb;
}
///
///deleteHitBox(hitbox)
VOID deleteHitBox(struct HitBox* hb)
{
  if (hb) {
    FreePooled(g_MemoryPool, hb, sizeof(struct HitBox));
  }
}
///
