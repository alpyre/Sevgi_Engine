///includes
#define ECS_SPECIFIC

#include <exec/exec.h>
#include <hardware/custom.h>

#include <proto/exec.h>

#include "SDI_headers/SDI_compiler.h"

#include "system.h"
#include "cop_inst_macros.h"
#include "color.h"
///
///defines
///
///globals
extern struct Custom custom;
///
///prototypes
VOID initColorTable(struct ColorTable* ct);
VOID initColorTable_CL(struct ColorTable* ct);
VOID initColorTable_GRD(struct ColorTable* ct);
VOID setColor(ULONG index, UBYTE R, UBYTE G, UBYTE B);
///

///newColorTable(palette, fade_steps, step)
/******************************************************************************
 * Allocates a new ColorTable an initializes it with the color values on the  *
 * given palette. Size of the ColorTable is defined by the number of colors   *
 * on the palette (the first byte of it holds number of colors - 1).          *
 * fade_steps value determines the number of frames a complete fade in/out    *
 * with this ColorTable takes (if you call updateColorTable() every frame).   *
 * This value can later be changed using changeFadeSteps() function.          *
 * The step value initializes the ColorTable to a specific frame of fade      *
 * in/out. (steps = 0 all black, steps = fade_steps actual colors)            *
 * Has to be freed by freeColorTable().                                       *
 ******************************************************************************/
struct ColorTable* newColorTable(UBYTE* palette, UWORD fade_steps, ULONG step)
{
  UWORD colors = *palette + 1;
  struct ColorTable* ct = (struct ColorTable*)AllocMem(sizeof(struct ColorTable), MEMF_ANY | MEMF_CLEAR);
  if (ct) {
    struct ColorState* cs = (struct ColorState*)AllocMem(sizeof(struct ColorState) * colors, MEMF_ANY | MEMF_CLEAR);
    if (cs) {
      ct->table  = palette;
      ct->colors = colors;
      ct->states = cs;
      ct->fade_steps = fade_steps;
      ct->fade_step = step;
      initColorTable(ct);
    }
    else {
      FreeMem(ct, sizeof(struct ColorTable));
      return NULL;
    }
  }

  return ct;
}
///
///newColorTable_CL(copperlist, fade_steps, step)
/******************************************************************************
 * Same as the above function but modified to work for the color instructions *
 * on copperlists.                                                            *
 * Traverses the copperlist and counts the color instructions on it and       *
 * creates a ColorTable to be able to fade the copper color changes.          *
 * WARNING: A table created with this variant of the function can be updated  *
 * with updateColorTable(). But MUST only be set with setColorTable_CL().     *
 * WARNING: Never use on a DYNAMIC_COPPERLIST which has scrollable/movable    *
 * gradients on its rainbow. Use the newColorTable_GRD() variant on its       *
 * gradients instead.                                                         *
 ******************************************************************************/
struct ColorTable* newColorTable_CL(UWORD* copperlist, UWORD fade_steps, ULONG step)
{
  struct ColorTable* ct;
  ULONG* inst_ptr = (ULONG*)copperlist;
  ULONG instruction = *inst_ptr;
  ULONG colors = 0;

  //count the color instructions on the copperlist
  while (instruction != END) {
    if (!(instruction & ~CI_MOVE_MASK)) {  // is this a move instruction
      ULONG address = instruction >> 16;
      if (address >= (COLOR00 & 0x01FE) && address <= (COLOR31 & 0x01FE)) colors++; // is this a color instruction
    }
    instruction = *(++inst_ptr);
  }

  ct = (struct ColorTable*)AllocMem(sizeof(struct ColorTable), MEMF_ANY | MEMF_CLEAR);
  if (ct) {
    struct ColorState* cs = (struct ColorState*)AllocMem(sizeof(struct ColorState) * colors, MEMF_ANY | MEMF_CLEAR);
    if (cs) {
      ct->table  = (UBYTE*)copperlist;
      ct->colors = colors;
      ct->states = cs;
      ct->fade_steps = fade_steps;
      ct->fade_step = step;
      initColorTable_CL(ct);
    }
    else {
      FreeMem(ct, sizeof(struct ColorTable));
      return NULL;
    }
  }

  return ct;
}
///
///newColorTable_GRD(copperlist, fade_steps, step)
/******************************************************************************
 * Same as the above function but modified to work for the color instructions *
 * on a gradient. Also initiates the colors on the gradient to the step given.*
 * WARNING: A table created with this variant of the function can be updated  *
 * with updateColorTable(). But MUST only be set with setColorTable_GRD().    *
 ******************************************************************************/
struct ColorTable* newColorTable_GRD(struct Gradient* grd, UWORD fade_steps, ULONG step)
{
  UWORD colors = grd->num_colors;
  struct ColorTable* ct = (struct ColorTable*)AllocMem(sizeof(struct ColorTable), MEMF_ANY | MEMF_CLEAR);
  if (ct) {
    struct ColorState* cs = (struct ColorState*)AllocMem(sizeof(struct ColorState) * colors, MEMF_ANY | MEMF_CLEAR);
    if (cs) {
      ct->table  = (UBYTE*)grd;
      ct->colors = colors;
      ct->states = cs;
      ct->fade_steps = fade_steps;
      ct->fade_step = step;
      initColorTable_GRD(ct);
      ct->state = 0xFFFF; //Just a value to skip CT_IDLE check on the call below
      setColorTable_GRD(ct);
      ct->state = CT_IDLE; //Now we can idle it as default
    }
    else {
      FreeMem(ct, sizeof(struct ColorTable));
      return NULL;
    }
  }

  return ct;
}
///
///freeColorTable(ColorTable)
/******************************************************************************
 * Deallocates the given ColorTable from memory.                              *
 ******************************************************************************/
VOID freeColorTable(struct ColorTable* ct)
{
  if (ct) {
    FreeMem(ct->states, sizeof(struct ColorState) * ct->colors);
    FreeMem(ct, sizeof(struct ColorTable));
  }
}
///

///initColorTable(color_table)
/******************************************************************************
 * Calculates and sets the increment and state values on the ColorTable       *
 * according to the color values on the palette linked.                       *
 * If the fade_step value is set to 0 all colors will be initialized to       *
 * 0 (black). If the fade_step value is initialized to fade_steps value all   *
 * colors will be initialized to the actual color on the linked palette.      *
 * This function will be called initially by the newColorTable() function.    *
 * You can set the step there!                                                *
 ******************************************************************************/
VOID initColorTable(struct ColorTable* ct)
{
  ULONG colors = ct->colors;
  ULONG i, l;

  for (i = 0, l = 1; i < colors; i++) {
    struct ColorState* cs = &ct->states[i];
    cs->increment.R = ((ct->table[l++] + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->increment.G = ((ct->table[l++] + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->increment.B = ((ct->table[l++] + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->color.value.R = cs->increment.R * ct->fade_step;
    cs->color.value.G = cs->increment.G * ct->fade_step;
    cs->color.value.B = cs->increment.B * ct->fade_step;
  }
}
///
///initColorTable_CL(color_table)
/******************************************************************************
 * Same as the above function but this one is modified to calculate and set   *
 * the increment and state values on the ColorTable according to the color    *
 * values on the copperlist linked instead.                                   *
 ******************************************************************************/
VOID initColorTable_CL(struct ColorTable* ct)
{
  ULONG colors = ct->colors;
  ULONG* cl_ptr = (ULONG*)ct->table;
  ULONG instruction = *cl_ptr;
  ULONG address = instruction >> 16;
  ULONG i;

  for (i = 0; i < colors; i++) {
    struct ColorState* cs = &ct->states[i];

    while (instruction & ~CI_MOVE_MASK || (address = (instruction >> 16)) < (COLOR00 & 0x01FE) || address > (COLOR31 & 0x01FE)) {
      instruction = *(++cl_ptr);
    }

    cs->increment.R = ((((instruction & 0x0F00) >> 8) + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->increment.G = ((((instruction & 0x00F0) >> 4) + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->increment.B = (( (instruction & 0x000F)       + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->color.value.R = cs->increment.R * ct->fade_step;
    cs->color.value.G = cs->increment.G * ct->fade_step;
    cs->color.value.B = cs->increment.B * ct->fade_step;

    instruction = *(++cl_ptr);
  }
}
///
///initColorTable_GRD(color_table)
/******************************************************************************
 * Calculates and sets the increment and state values on the ColorTable       *
 * for the colors on a gradient.                                              *
 * Supports combining the two separate 12bit values on the AGA gradients.     *
 * If the fade_step value is set to 0 all colors will be initialized to       *
 * 0 (black). If the fade_step value is initialized to fade_steps value all   *
 * colors will be initialized to the actual color on the linked gradient.     *
 * This function will be called initially by the newColorTable_GRD() function.*
 * You can set the step there!                                                *
 ******************************************************************************/
VOID initColorTable_GRD(struct ColorTable* ct)
{
  ULONG colors = ct->colors;
  struct Gradient* grd = (struct Gradient*)ct->table;
  ULONG i, l;

  for (i = 0, l = 0; i < colors; i++) {
    struct ColorState* cs = &ct->states[i];
    ULONG R,G,B;
    R = (grd->colors[l] & 0x0F00) >> 4;
    G =  grd->colors[l] & 0x00F0;
    B = (grd->colors[l] & 0x000F) << 4;

    if (grd->type == GRD_TYPE_AGA) {
      l++;
      R |= (grd->colors[l] & 0x0F00) >> 8;
      G |= (grd->colors[l] & 0x00F0) >> 4;
      B |=  grd->colors[l] & 0x000F;
    }

    cs->increment.R = ((R + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->increment.G = ((G + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->increment.B = ((B + 1) << CT_PRECISION) / ct->fade_steps - 1;
    cs->color.value.R = cs->increment.R * ct->fade_step;
    cs->color.value.G = cs->increment.G * ct->fade_step;
    cs->color.value.B = cs->increment.B * ct->fade_step;

    l++;
  }
}
///
///changeFadeSteps(color_table, steps)
/******************************************************************************
 * Changes the fade steps of a ColorTable. Fade steps is how many frames a    *
 * complete fade in/out to/from a color palette will take.                    *
 * WARNING: A value of 1 will cause a division by zero error!                 *                                                           *
 ******************************************************************************/
VOID changeFadeSteps(struct ColorTable* ct, ULONG steps)
{
  struct ColorState* cs, *cs_end;
  UWORD old_steps = ct->fade_steps;
  UWORD new_steps = steps - 1;

  ct->fade_step = ct->fade_step * steps / old_steps--;
  ct->fade_steps = steps;

  for (cs = ct->states, cs_end = cs + ct->colors; cs < cs_end; cs++) {
    cs->increment.R = (cs->increment.R * old_steps) / new_steps;
    cs->increment.G = (cs->increment.G * old_steps) / new_steps;
    cs->increment.B = (cs->increment.B * old_steps) / new_steps;
  }
}
///

///updateColorTable(color_table)
/******************************************************************************
 * Updates the color states on the given ColorTable according to the state it *
 * is in.                                                                     *
 * start: the first color to be updated in the table (zero indexed).          *
 * end: the last color to be updated PLUS ONE!!!                              *
 * Doesn't require to be called in the VBL.                                   *
 * Calling setColorTable() later on the VBL will animate the fade in/out.     *
 ******************************************************************************/
VOID updateColorTable_Partial(struct ColorTable* ct, ULONG start, ULONG end)
{
  switch (ct->state) {
    case CT_IDLE:
      return;
    break;
    case CT_FADE_IN:
    {
      struct ColorState* cs, *cs_end;

      if (ct->fade_step == ct->fade_steps) {
        ct->state = CT_IDLE;
        return;
      }
      ct->fade_step++;
      for (cs = ct->states + start, cs_end = cs + end; cs < cs_end; cs++) {
        cs->color.value.R += cs->increment.R;
        cs->color.value.G += cs->increment.G;
        cs->color.value.B += cs->increment.B;
      }
    }
    break;
    case CT_FADE_OUT:
    {
      struct ColorState* cs, *cs_end;

      if (ct->fade_step == 0) {
        ct->state = CT_IDLE;
        return;
      }
      ct->fade_step--;

      for (cs = ct->states + start, cs_end = cs + end; cs < cs_end; cs++) {
        cs->color.value.R -= cs->increment.R;
        cs->color.value.G -= cs->increment.G;
        cs->color.value.B -= cs->increment.B;
      }
    }
    break;
  }
}
///

///setColorTable(color_table)
/******************************************************************************
 * Updates color registers with the current state of the ColorTable.          *
 * Designed to be called during the VBL.                                      *
 * start: the first color to be set from the palette (zero indexed).          *
 * end: the last color to be set PLUS ONE!!!                                  *
 * setColorTable(ct) variant will set all of the colors from the colortable.  *
 * WARNING: Will be called from an interrupt!                                 *
 ******************************************************************************/
VOID setColorTable_REG(struct ColorTable* ct, ULONG start, ULONG end)
{
  struct ColorState* cs, *cs_end;
  UWORD* color;
  UWORD* reg_start;
  #ifdef CT_AGA
  ULONG start_mod = start % 32;
  ULONG bank, bank_start, banks_max;
  #endif

  if (ct->state == CT_IDLE) return;

  cs = ct->states + start;
  cs_end = ct->states + end;

  #ifdef CT_AGA
  reg_start = custom.color + start_mod;
  #else
  reg_start = custom.color + start;
  #endif

  #ifdef CT_AGA
    for (bank = bank_start = start >> 5, banks_max = end >> 5; bank <= banks_max; bank++, reg_start = custom.color) {
      custom.bplcon3 = BPLCON3_V | (bank << 13);
  #endif
      for (color = reg_start; cs < cs_end && color < &custom.htotal; cs++, color++) {
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R << 4) & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
        #else
          *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
        #endif
      }
  #ifdef CT_AGA
    }
    cs = ct->states + start;

    reg_start = custom.color + start_mod;

    for (bank = bank_start; bank <= banks_max; bank++, reg_start = custom.color) {
      custom.bplcon3 = BPLCON3_V | BPLCON3_LOCT | (bank << 13);
      for (color = reg_start; cs < cs_end && color < &custom.htotal; cs++, color++) {
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R & 0x0F) << 8) | ((cs->color.bytes.G & 0x0F) << 4) | (cs->color.bytes.B & 0x0F);
        #else
          *color = ((UWORD)cs->color.bytes.RL << 4 & 0xF00) | (cs->color.bytes.GL & 0xF0) | (cs->color.bytes.BL >> 4);
        #endif
      }
    }
    //custom.bplcon3 = BPLCON3_V;
  #endif
}
///
///setColorTable_CL(color_table)
/******************************************************************************
 * Updates the CopperList with the current state of the ColorTable.           *
 * Designed to be called during the VBL.                                      *
 * NOTE: Is not designed to fade as smooth as AGA is capable (24 bits). Fades *
 * will be as smooth as OCS fades (12 bits). Designing 24 bit smooth fades    *
 * will require a re-design of newColorTable_CL() and initColorTable_CL() as  *
 * well and be more expensive.                                                *
 * WARNING: Will be called from an interrupt!                                 *
 ******************************************************************************/
VOID setColorTable_CL(struct ColorTable* ct)
{
  if (ct->state == CT_IDLE) return;
  else {
    struct ColorState* cs, *cs_end;
    ULONG* cl_ptr = (ULONG*)ct->table;
    ULONG instruction = *cl_ptr;
    UWORD address = instruction >> 16;

    for (cs = ct->states, cs_end = cs + ct->colors; cs < cs_end; cs++) {
      UWORD* color;

      while (instruction & ~CI_MOVE_MASK || (address = (instruction >> 16)) < (COLOR00 & 0x01FE) || address > (COLOR31 & 0x01FE)) {
        instruction = *(++cl_ptr);
      }

      color = (UWORD*)cl_ptr + 1;

      #if CT_PRECISION == 8
      *color = (((UWORD)cs->color.bytes.R & 0x0F) << 8) | ((cs->color.bytes.G & 0x0F) << 4) | (cs->color.bytes.B & 0x0F);
      #else
      *color = ((UWORD)cs->color.bytes.RL << 4 & 0xF00) | (cs->color.bytes.GL & 0xF0) | (cs->color.bytes.BL >> 4);
      #endif

      instruction = *(++cl_ptr);
    }
  }
}
///
///setColorTable_GRD(color_table)
/******************************************************************************
 * Updates the gradient with the current state of the ColorTable.             *
 * Is designed to do the smooth AGA (24 bits) fades.                          *
 ******************************************************************************/
VOID setColorTable_GRD(struct ColorTable* ct)
{
  if (ct->state == CT_IDLE) return;
  else {
    struct ColorState* cs, *cs_end;
    struct Gradient* grd = (struct Gradient*)ct->table;
    UWORD* color = grd->colors;

    if (grd->type == GRD_TYPE_AGA) {
      for (cs = ct->states, cs_end = cs + ct->colors; cs < cs_end; cs++) {

        #if CT_PRECISION == 8
          *color = ((UWORD)cs->color.bytes.R << 4 & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
        #elif CT_PRECISION == 20
          *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
        #endif

        color++;

        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R & 0x0F) << 8) | ((cs->color.bytes.G & 0x0F) << 4) | (cs->color.bytes.B & 0x0F);
        #elif CT_PRECISION == 20
          *color = ((UWORD)cs->color.bytes.RL << 4 & 0xF00) | (cs->color.bytes.GL & 0xF0) | (cs->color.bytes.BL >> 4);
        #endif

        color++;
      }
    }
    else
    {
      for (cs = ct->states, cs_end = cs + ct->colors; cs < cs_end; cs++) {
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R << 4) & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
        #elif CT_PRECISION == 20
          *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
        #endif

        color++;
      }
    }
  }
}
///
///setColorTable_CLP(color_table, address, start, end)
/******************************************************************************
 * Updates color instructions of the color palette section of a copperlist    *
 * (CLP) with the current state of the ColorTable.                            *
 * The color palette section MUST be in the form below:                       *
 * For OCS/ECS (CT_AGA not defined)                                           *
 *   ...                                                                      *
 *   MOVE_PH(COLOR00, 0x0),                                                   *
 *   MOVE(COLOR01, 0x0),                                                      *
 *   MOVE(COLOR02, 0x0),                                                      *
 *   ... (up to palette size)                                                 *
 *                                                                            *
 * For AGA (CT_AGA defined)                                                   *
 *   ...                                                                      *
 *   MOVE_PH(BPLCON3, BPLCON3_V),                         // bank 0           *
 *   MOVE(COLOR00, 0x0),                                                      *
 *   MOVE(COLOR01, 0x0),                                                      *
 *   MOVE(COLOR02, 0x0),                                                      *
 *   ... (up to palette size or COLOR31 if there are more than 32 colors)     *
 *   MOVE(BPLCON3, BPLCON3_V | (1 << 13)),                // bank 1           *
 *   MOVE(COLOR00, 0x0),                                                      *
 *   MOVE(COLOR01, 0x0),                                                      *
 *   ... (up to palette size or COLOR31 if there are more than 64 colors)     *
 *   ...                                                                      *
 *   MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT),             // bank 0 LOCT      *
 *   MOVE(COLOR00, 0x0),                                                      *
 *   MOVE(COLOR01, 0x0),                                                      *
 *   ...                                                                      *
 *   MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT | (1 << 13)), // bank 1 LOCT      *
 *   MOVE(COLOR00, 0x0),                                                      *
 *   MOVE(COLOR01, 0x0),                                                      *
 *   ...                                                                      *
 *                                                                            *
 * The color instructions MUST cover the number of colors on the color_table. *
 * This function only updates the color values. The move instructions for     *
 * bank and LOCT selection MUST initially be set correctly on the array as    *
 * shown above.                                                               *
 * Designed to be called during the VBL.                                      *
 * address: the CL_PALETTE value set by allocCopperList() to the _PH above.   *
 *          in DYNAMIC_COPPERLIST mode CL_PALETTE is an offset!               *
 * start: the first color to be set from the palette (zero indexed).          *
 * end: the last color to be set PLUS ONE!!!                                  *
 ******************************************************************************/
#ifdef USE_CLP
VOID setColorTable_CLP(struct ColorTable* ct, UWORD* address, ULONG start, ULONG end)
{
  if (ct->state == CT_IDLE) return;
  else {
#ifdef CT_AGA
    #define BANK_SIZE 132 // (1 bank inst + 32 color insts) x inst size
    struct ColorState* cs, *cs_end;
    ULONG instruction;
    ULONG inst_start;
    ULONG inst_end;
    ULONG start_mod = start % 32;
    ULONG bank, bank_start, banks_max;
    ULONG num_banks = (ct->colors + 31) >> 5;
    ULONG loct_start = ((ULONG)address) + ((num_banks + ct->colors) * 4);

    cs = ct->states + start;
    cs_end = ct->states + end;
    bank_start = start >> 5;
    inst_start = ((ULONG)address) + 4 + (bank_start * BANK_SIZE) + start_mod * 4;
    inst_end = ((ULONG)address) + 4 + ((bank_start + 1) * BANK_SIZE);

    for (bank = bank_start, banks_max = end >> 5; bank <= banks_max; bank++, inst_start = ((ULONG)address) + 4 + bank * BANK_SIZE, inst_end += BANK_SIZE) {
      for (instruction = inst_start; cs < cs_end && instruction < inst_end; cs++, instruction += 4) {
        UWORD* color = (UWORD*)instruction;
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R << 4) & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
        #else
          *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
        #endif
      }
    }

    cs = ct->states + start;
    inst_start = loct_start + 4 + (bank_start * BANK_SIZE) + start_mod * 4;
    inst_end = loct_start + 4 + ((bank_start + 1) * BANK_SIZE);

    for (bank = bank_start; bank <= banks_max; bank++, inst_start = loct_start + 4 + bank * BANK_SIZE, inst_end += BANK_SIZE) {
      for (instruction = inst_start; cs < cs_end && instruction < inst_end; cs++, instruction += 4) {
        UWORD* color = (UWORD*)instruction;
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R & 0x0F) << 8) | ((cs->color.bytes.G & 0x0F) << 4) | (cs->color.bytes.B & 0x0F);
        #else
          *color = ((UWORD)cs->color.bytes.RL << 4 & 0xF00) | (cs->color.bytes.GL & 0xF0) | (cs->color.bytes.BL >> 4);
        #endif
      }
    }
#else //CT_AGA
    struct ColorState* cs, *cs_end;
    ULONG instruction;
    ULONG inst_start;

    cs = ct->states + start;
    cs_end = ct->states + end;
    inst_start = ((ULONG)address) + start * 4;

    for (instruction = inst_start; cs < cs_end; cs++, instruction += 4) {
      UWORD* color = (UWORD*)instruction;
      #if CT_PRECISION == 8
        *color = (((UWORD)cs->color.bytes.R << 4) & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
      #else
        *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
      #endif
    }
#endif //CT_AGA
  }
}

/******************************************************************************
 * The ECS specific variant of setColorTable_CLP(). You can uncomment and use *
 * this variant if you need to create a custom OCS/ECS display on an AGA game.*                                                                           *
 ******************************************************************************/
/*
VOID setColorTable_CLP_ECS(struct ColorTable* ct, UWORD* address, ULONG start, ULONG end)
{
  if (ct->state == CT_IDLE) return;
  else {
    struct ColorState* cs, *cs_end;
    ULONG instruction;
    ULONG inst_start;

    cs = ct->states + start;
    cs_end = ct->states + end;
    inst_start = ((ULONG)address) + start * 4;

    for (instruction = inst_start; cs < cs_end; cs++, instruction += 4) {
      UWORD* color = (UWORD*)instruction;
      #if CT_PRECISION == 8
        *color = (((UWORD)cs->color.bytes.R << 4) & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
      #else
        *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
      #endif
    }
  }
}
*/

/******************************************************************************
 * The AGA specific variant of setColorTable_CLP(). You can uncomment and use *
 * this variant if you need to create a custom AGA display on an OCS/ECS game.*
 ******************************************************************************/
/*
VOID setColorTable_CLP_AGA(struct ColorTable* ct, UWORD* address, ULONG start, ULONG end)
{
  if (ct->state == CT_IDLE) return;
  else {
    #define BANK_SIZE 132 // (1 bank inst + 32 color insts) x inst size
    struct ColorState* cs, *cs_end;
    ULONG instruction;
    ULONG inst_start;
    ULONG inst_end;
    ULONG start_mod = start % 32;
    ULONG bank, bank_start, banks_max;
    ULONG num_banks = (ct->colors + 31) >> 5;
    ULONG loct_start = ((ULONG)address) + ((num_banks + ct->colors) * 4);

    cs = ct->states + start;
    cs_end = ct->states + end;
    bank_start = start >> 5;
    inst_start = ((ULONG)address) + 4 + (bank_start * BANK_SIZE) + start_mod * 4;
    inst_end = ((ULONG)address) + 4 + ((bank_start + 1) * BANK_SIZE);

    for (bank = bank_start, banks_max = end >> 5; bank <= banks_max; bank++, inst_start = ((ULONG)address) + 4 + bank * BANK_SIZE, inst_end += BANK_SIZE) {
      for (instruction = inst_start; cs < cs_end && instruction < inst_end; cs++, instruction += 4) {
        UWORD* color = (UWORD*)instruction;
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R << 4) & 0xF00) | (cs->color.bytes.G & 0xF0) | (cs->color.bytes.B >> 4);
        #else
          *color = ((UWORD)cs->color.bytes.RH << 8) | (cs->color.bytes.GH << 4) | cs->color.bytes.BH;
        #endif
      }
    }

    cs = ct->states + start;
    inst_start = loct_start + 4 + (bank_start * BANK_SIZE) + start_mod * 4;
    inst_end = loct_start + 4 + ((bank_start + 1) * BANK_SIZE);

    for (bank = bank_start; bank <= banks_max; bank++, inst_start = loct_start + 4 + bank * BANK_SIZE, inst_end += BANK_SIZE) {
      for (instruction = inst_start; cs < cs_end && instruction < inst_end; cs++, instruction += 4) {
        UWORD* color = (UWORD*)instruction;
        #if CT_PRECISION == 8
          *color = (((UWORD)cs->color.bytes.R & 0x0F) << 8) | ((cs->color.bytes.G & 0x0F) << 4) | (cs->color.bytes.B & 0x0F);
        #else
          *color = ((UWORD)cs->color.bytes.RL << 4 & 0xF00) | (cs->color.bytes.GL & 0xF0) | (cs->color.bytes.BL >> 4);
        #endif
      }
    }
  }
}
*/
#endif //USE_CLP
///

///blackOut()
/******************************************************************************
 * Sets all color registers to black.                                         *
 * This function isn't a part of the fade routine.                            *
 ******************************************************************************/
VOID blackOut()
{
  #ifdef CT_AGA
  ULONG b;
  #endif
  ULONG c;

  #ifdef CT_AGA
  for (b = 0; b < 8; b++) {
    custom.bplcon3 = BPLCON3_V | (b << 13);
  #endif

    for (c = 0; c < 32; c++) {
      custom.color[c] = 0;
    }

  #ifdef CT_AGA
  }
  #endif
}
///
///setColorToAll(R, G, B)
/******************************************************************************
 * Sets all color registers to the specified RGB value.                       *
 * This function isn't a part of the fade routine.                            *
 ******************************************************************************/
VOID setColorToAll(UBYTE R, UBYTE G, UBYTE B)
{
#ifdef CT_AGA
  UWORD color_v = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  UWORD color_v_loct = ((UWORD)R << 8 & 0xF00) | (G << 4 & 0xF0) | (B & 0xF);
  ULONG b, c;

  for (b = 0; b < 8; b++) {
    custom.bplcon3 = BPLCON3_V | (b << 13);

    for (c = 0; c < 32; c++) {
      custom.color[c] = color_v;
    }
  }

  for (b = 0; b < 8; b++) {
    custom.bplcon3 = BPLCON3_V | BPLCON3_LOCT | (b << 13);

    for (c = 0; c < 32; c++) {
      custom.color[c] = color_v_loct;
    }
  }
#else //CT_AGA
  UWORD color_v = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  ULONG c;

  for (c = 0; c < 32; c++) {
    custom.color[c] = color_v;
  }
#endif //CT_AGA
}
///
///setColorToAll_CLP(address, size, R, G, B)
/******************************************************************************
 * Sets all color instructions on a CLP (palette on copperlist) to the        *
 * specified RGB value.                                                       *
 * address: the CL_PALETTE value got from allocCopperList().                  *
 *          in DYNAMIC_COPPERLIST mode CL_PALETTE is an offset!               *
 * size   : number of colors on the palette on copperlist                     *
 * This function isn't a part of the fade routine.                            *
 ******************************************************************************/
#ifdef USE_CLP
VOID setColorToAll_CLP(UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B)
{
#ifdef CT_AGA
  ULONG num_banks = (size + 31) >> 5;
  UWORD color_v = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  UWORD color_v_loct = ((UWORD)R << 8 & 0xF00) | (G << 4 & 0xF0) | (B & 0xF);
  ULONG instruction = (ULONG)address;
  ULONG instruction_max;
  ULONG b, c;

  for (b = 0, c = 0; b < num_banks; b++) {
    instruction += 4; //skip the bank set instruction (MOVE(BPLCON3...)
    instruction_max = instruction + 128; // 32 * 4
    for (; c < size && instruction < instruction_max; c++, instruction += 4) {
      UWORD* color = (UWORD*)instruction;
      *color = color_v;
    }
  }

  for (b = 0, c = 0; b < num_banks; b++) {
    instruction += 4; //skip the bank set instruction (MOVE(BPLCON3...)
    instruction_max = instruction + 128; // 32 * 4
    for (; c < size && instruction < instruction_max; c++, instruction += 4) {
      UWORD* color = (UWORD*)instruction;
      *color = color_v_loct;
    }
  }
#else //CT_AGA
  UWORD color_v = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  ULONG instruction;
  ULONG instruction_max = instruction + size * 4;

  for (instruction = (ULONG)address; instruction < instruction_max; instruction += 4) {
    UWORD* color = (UWORD*)instruction;
    *color = color_v;
  }
#endif //CT_AGA
}

/******************************************************************************
 * The ECS specific variant of setColorToAll_CLP(). You can uncomment and use *
 * this variant if you need to create a custom OCS/ECS display on an AGA game.*                                                                           *
 ******************************************************************************/
/*
VOID setColorToAll_CLP_ECS(UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B)
{
  UWORD color_v = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  ULONG instruction;
  ULONG instruction_max = instruction + size * 4;

  for (instruction = (ULONG)address; instruction < instruction_max; instruction += 4) {
    UWORD* color = (UWORD*)instruction;
    *color = color_v;
  }
}
*/

/******************************************************************************
 * The AGA specific variant of setColorToAll_CLP(). You can uncomment and use *
 * this variant if you need to create a custom AGA display on an OCS/ECS game.*
 ******************************************************************************/
/*
VOID setColorToAll_CLP_AGA(UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B)
{
  ULONG num_banks = (size + 31) >> 5;
  UWORD color_v = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  UWORD color_v_loct = ((UWORD)R << 8 & 0xF00) | (G << 4 & 0xF0) | (B & 0xF);
  ULONG instruction = (ULONG)address;
  ULONG instruction_max;
  ULONG b, c;

  for (b = 0, c = 0; b < num_banks; b++) {
    instruction += 4; //skip the bank set instruction (MOVE(BPLCON3...)
    instruction_max = instruction + 128; // 32 * 4
    for (; c < size && instruction < instruction_max; c++, instruction += 4) {
      UWORD* color = (UWORD*)instruction;
      *color = color_v;
    }
  }

  for (b = 0, c = 0; b < num_banks; b++) {
    instruction += 4; //skip the bank set instruction (MOVE(BPLCON3...)
    instruction_max = instruction + 128; // 32 * 4
    for (; c < size && instruction < instruction_max; c++, instruction += 4) {
      UWORD* color = (UWORD*)instruction;
      *color = color_v_loct;
    }
  }
}
*/
#endif //USE_CLP
///
///setColor(index, R, G, B)
/******************************************************************************
 * Directly sets the given color register to the given RGB value.             *
 * Does not update the ColorTable and not optimized to be called from a loop. *
 * This function isn't a part of the fade routine.                            *
 ******************************************************************************/
INLINE VOID setColor(ULONG index, UBYTE R, UBYTE G, UBYTE B)
{
  #ifdef CT_AGA
    ULONG bank = (index / 32) << 13;
    index %= 32;
    custom.bplcon3 = BPLCON3_V | bank;
  #endif

  custom.color[index] = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  #ifdef CT_AGA
  custom.bplcon3 = BPLCON3_V | BPLCON3_LOCT | bank;
  custom.color[index] = ((UWORD)R << 8 & 0xF00) | (G << 4 & 0xF0) | (B & 0xF);
  custom.bplcon3 = BPLCON3_V;
  #endif
}
///
///setColor_CLP(index, address, size, R, G, B)
/******************************************************************************
 * Sets the color at the given index of a CLP (palette on copperlist) at the  *
 * given address to the given RGB value.                                      *
 * index  : the index of the color on the palette (0 indexed).                *
 * address: the CL_PALETTE value set by allocCopperList() to the _PH above.   *
 *          in DYNAMIC_COPPERLIST mode CL_PALETTE is an offset!               *
 * size   : number of colors on the palette on copperlist (only in AGA)       *
 * Does not update the ColorTable and not optimized to be called from a loop. *
 * This function isn't a part of the fade routine.                            *
 ******************************************************************************/
#ifdef USE_CLP
INLINE VOID setColor_CLP(ULONG index, UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B)
{
#ifdef CT_AGA
  ULONG num_banks = (size + 31) >> 5;
  ULONG bank = index / 32;
  UWORD* color = (UWORD*)((ULONG)address + (index + bank + 1) * 4);
  UWORD* color_loct = (UWORD*)((ULONG)color + ((num_banks + size) * 4));

  *color = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  *color_loct = ((UWORD)R << 8 & 0xF00) | (G << 4 & 0xF0) | (B & 0xF);
#else //CT_AGA
  UWORD* color = (UWORD*)((ULONG)address + index * 4);
  *color = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
#endif //CT_AGA
}

/******************************************************************************
 * The ECS specific variant of setColor_CLP(). You can uncomment and use this *
 * variant if you need to create a custom OCS/ECS display on an AGA game.     *                                                                           *
 ******************************************************************************/
/*
INLINE VOID setColor_CLP_ECS(ULONG index, UWORD* address, UBYTE R, UBYTE G, UBYTE B)
{
  UWORD* color = (UWORD*)((ULONG)address + index * 4);
  *color = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
}
*/

/******************************************************************************
 * The AGA specific variant of setColor_CLP(). You can uncomment and use this *
 * variant if you need to create a custom AGA display on an OCS/ECS game.     *
 ******************************************************************************/
/*
INLINE VOID setColor_CLP_AGA(ULONG index, UWORD* address, ULONG size, UBYTE R, UBYTE G, UBYTE B)
{
  ULONG num_banks = (size + 31) >> 5;
  ULONG bank = index / 32;
  UWORD* color = (UWORD*)((ULONG)address + (index + bank + 1) * 4);
  UWORD* color_loct = (UWORD*)((ULONG)color + ((num_banks + size) * 4));

  *color = ((UWORD)R << 4 & 0xF00) | (G & 0xF0) | (B >> 4);
  *color_loct = ((UWORD)R << 8 & 0xF00) | (G << 4 & 0xF0) | (B & 0xF);
}
*/
#endif //USE_CLP
///
///setColors(palette)
/******************************************************************************
 * Sets the colors in the given palette directly into the color registers.    *
 * Does not update the ColorTable and is not optimized of a loop.             *
 * This function isn't a part of the fade routine.                            *
 ******************************************************************************/
VOID setColors(UBYTE* palette)
{
  ULONG i;
  UBYTE* c;
  UWORD colors = *palette + 1;

  for (i = 0, c = palette + 1; i < colors; i++, c += 3) {
    setColor(i, *c, *(c + 1), *(c + 2));
  }
}
///
///updateColor(color_table, index, R, G, B)
/******************************************************************************
 * Sets the color at the given index on the given ColorTable to the given RGB *
 * value. This includes recalculating the increment and state values for the  *
 * color.                                                                     *
 * It also updates the color value on the linked palette and directly sets    *
 * the color on the corresponding color register as well.                     *
 * This function isn't optimized to be called from a loop. It is designed to  *
 * set just a single color index to a different color.                        *
 * Fade engine uses updateColorTable() and setColorTable() functions instead. *
 * WARNING: Do NOT use this function on ColorTables that are specific to      *
 * other structures like Gradients or CopperLists.                            *
 ******************************************************************************/
VOID updateColor(struct ColorTable* ct, ULONG index, UBYTE R, UBYTE G, UBYTE B)
{
  struct ColorState* cs = &ct->states[index];
  UBYTE* plt_ptr = &ct->table[index];
  *++plt_ptr = R;
  *++plt_ptr = G;
  *++plt_ptr = B;

  cs->increment.R = ((R + 1) << CT_PRECISION) / ct->fade_steps - 1;
  cs->increment.G = ((G + 1) << CT_PRECISION) / ct->fade_steps - 1;
  cs->increment.B = ((B + 1) << CT_PRECISION) / ct->fade_steps - 1;
  cs->color.value.R = cs->increment.R * ct->fade_step;
  cs->color.value.G = cs->increment.G * ct->fade_step;
  cs->color.value.B = cs->increment.B * ct->fade_step;
#ifndef USE_CLP
  setColor(index, R, G, B);
#endif //USE_CLP
}
///
