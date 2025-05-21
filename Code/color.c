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
  setColor(index, R, G, B);
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
