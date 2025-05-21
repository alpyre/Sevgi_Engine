///includes
#include <exec/exec.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>

#include "system.h"
#include "cop_inst_macros.h"

#include "rainbow.h"
///
///defines
#define DISPLAY_START (DIWSTART_V >> 8)
#define SCREEN_START (DISPLAY_START + TOP_PANEL_HEIGHT)
#define SCREEN_END (SCREEN_START + SCREEN_HEIGHT)
///
///globals
extern struct Custom custom;
///
///prototypes
VOID initGradientBlit(VOID);
///

///createGradient(type, col_reg, num_colors, scr_pos, vis, pos, flags, huelist)
/******************************************************************************
 * Creates a scrollable gradient of Amiga hardware color values to be blitted *
 * into rainbows. AGA gradients use twice the size and every other color      *
 * value is to be set with LOCT bit set.                                      *
 * The gradient is created between the colors passed in huelist. A huelist is *
 * identical to palettes used creating ColorTables. First byte is the number  *
 * of colors in the huelist - 1. Following bytes are 24bit RGB values.        *
 * NOTE: Dithering algorithm is not implemented yet!                          *
 * WARNING: A valid huelist MUST always contain at least 2 colors!            *
 * WARNING: Division by zero on col_per_hue is not corner cased!              *
 * type: GRD_TYPE_OCS or GRD_TYPE_AGA                                         *
 * col_reg: On which color register this gradient will apply                  *
 * num_colors: number of colors on the gradient (total height of the gradient)*
 * scr_pos: the rasterline this gradient will start (as screen coordinate)    *
 * vis: visible height of the gradient                                        *
 * pos: position of the visible part                                          *
 * flags:                                                                     *
 *   GRD_SCROLLABLE: implies pos can be changed                               *
 *   GRD_MOVABLE   : implies scr_pos can be changed                           *
 *   GRD_BLITABLE  : gradient can be blitted on to a rainbow                  *
 *   GRD_DITHERED  : TODO: not implemented                                    *
 * huelist: start/end (and middle) colors that define the gradient. Has       *
 * identical format with color palettes.                                      *
 ******************************************************************************/
struct Gradient* createGradient(UBYTE type, UBYTE col_reg, UWORD num_colors, UWORD scr_pos, UWORD vis, UWORD pos, ULONG flags, UBYTE* huelist)
{
  struct Gradient* grd = (struct Gradient*)AllocMem(sizeof(struct Gradient), MEMF_ANY | MEMF_CLEAR);
  if (grd) {
    UWORD* colors = (UWORD*)AllocMem(num_colors * type * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if (colors) {
      if (huelist) {
        #define PRECISION 20
        struct {
          union {
            struct {
              ULONG R,G,B;
            }value;
            struct {
              UBYTE RH,RL;
              UWORD RP;
              UBYTE GH,GL;
              UWORD GP;
              UBYTE BH,BL;
              UWORD BP;
            }bytes;
          }color;
          struct {
            LONG R,G,B;
          }increment;
        }cs;
        ULONG num_hues = *huelist + 1;
        UBYTE* h_ptr = huelist + 1;
        LONG col_per_hue; //NOTE: Better naming required
        LONG h = 0, c = 0, c_end;

        for (h = 0; h < num_hues - 1; h++) {
          if (h == num_hues - 2) col_per_hue = num_colors - c - 1;
          else col_per_hue = (num_colors / (num_hues - 1));

          cs.color.value.R = *h_ptr << PRECISION; h_ptr++;
          cs.color.value.G = *h_ptr << PRECISION; h_ptr++;
          cs.color.value.B = *h_ptr << PRECISION; h_ptr++;
          if (*h_ptr == cs.color.value.R) cs.increment.R = 0;
          else cs.increment.R = ((*h_ptr << PRECISION) - (LONG)cs.color.value.R) / col_per_hue;
          h_ptr++;

          if (*h_ptr == cs.color.value.G) cs.increment.G = 0;
          else cs.increment.G = ((*h_ptr << PRECISION) - (LONG)cs.color.value.G) / col_per_hue;
          h_ptr++;

          if (*h_ptr == cs.color.value.B) cs.increment.B = 0;
          else cs.increment.B = ((*h_ptr << PRECISION) - (LONG)cs.color.value.B) / col_per_hue;
          h_ptr -= 2;

          for (c_end = c + col_per_hue; c < c_end; c++) {
            //NOTE: Implement dithering algorithm here!

            if (type == GRD_TYPE_AGA) {
              colors[c * 2] = (UWORD)cs.color.bytes.RH << 8 | cs.color.bytes.GH << 4 | cs.color.bytes.BH;
              colors[c * 2 + 1] = ((UWORD)cs.color.bytes.RL << 4 & 0xF00) | (cs.color.bytes.GL & 0xF0) | (cs.color.bytes.BL >> 4);
            }
            else
              colors[c] = (UWORD)cs.color.bytes.RH << 8 | cs.color.bytes.GH << 4 | cs.color.bytes.BH;

            cs.color.value.R += cs.increment.R;
            cs.color.value.G += cs.increment.G;
            cs.color.value.B += cs.increment.B;
          }
          if (h == num_hues - 2) {
            cs.color.value.R = *h_ptr << PRECISION; h_ptr++;
            cs.color.value.G = *h_ptr << PRECISION; h_ptr++;
            cs.color.value.B = *h_ptr << PRECISION; h_ptr++;

            if (type == GRD_TYPE_AGA) {
              colors[c * 2] = (UWORD)cs.color.bytes.RH << 8 | cs.color.bytes.GH << 4 | cs.color.bytes.BH;
              colors[c * 2 + 1] = ((UWORD)cs.color.bytes.RL << 4 & 0xF00) | (cs.color.bytes.GL & 0xF0) | (cs.color.bytes.BL >> 4);
            }
            else
              colors[c] = (UWORD)cs.color.bytes.RH << 8 | cs.color.bytes.GH << 4 | cs.color.bytes.BH;
          }
        }
      }

      grd->num_colors = num_colors;
      grd->type = type;
      grd->col_reg = col_reg;
      grd->scrollable = flags & (GRD_SCROLLABLE | GRD_MOVABLE) ? 1 : 0;
      grd->movable = (flags & GRD_MOVABLE) ? 1 : 0;
      grd->blitable = grd->scrollable ? 1 : 0;
      grd->vis = vis ? vis : (num_colors < SCREEN_HEIGHT ? num_colors : SCREEN_HEIGHT);
      grd->scr_pos_max = SCREEN_HEIGHT - grd->vis;
      grd->scr_pos = scr_pos < grd->scr_pos_max ? scr_pos : grd->scr_pos_max;
      grd->pos_max = num_colors - grd->vis;
      grd->pos = grd->scrollable ? (pos < grd->pos_max ? pos : grd->pos_max) : 0;
      grd->colors = colors;
    }
    else {
      FreeMem(grd, sizeof(struct Gradient));
      grd = NULL;
    }
  }

  return grd;
}
///
///scrollGradientUp(gradient, pixels)
/******************************************************************************
 * Scrolls a gradient up the given pixels clamping to proper boundaries.      *
 ******************************************************************************/
VOID scrollGradientUp(struct Gradient* grd, UWORD pixels)
{
  if (pixels > grd->pos) {
    grd->pos = 0;
  }
  else {
    grd->pos -= pixels;
  }
}
///
///scrollGradientDown(gradient, pixels)
/******************************************************************************
 * Scrolls a gradient down the given pixels clamping to proper boundaries.    *
 ******************************************************************************/
VOID scrollGradientDown(struct Gradient* grd, UWORD pixels)
{
  grd->pos += pixels;
  if (grd->pos > grd->pos_max) {
    grd->pos = grd->pos_max;
  }
}
///
///setGradientScrollPos(gradient, pos)
/******************************************************************************
 * Sets the scroll of a gradient to the given pos clamping to its boundaries. *
 ******************************************************************************/
VOID setGradientScrollPos(struct Gradient* grd, UWORD pos)
{
  if (pos > grd->pos_max) {
    grd->pos = grd->pos_max;
  }
  else {
    grd->pos = pos;
  }
}
///
///moveGradient(gradient, scr_pos)
/******************************************************************************
 * Changes the screen position of a gradient to the given new position..      *
 * The value is a screen coordinate and is clamped to the valid boundary.     *
 ******************************************************************************/
VOID moveGradient(struct Gradient* grd, UWORD scr_pos)
{
  if (scr_pos > grd->scr_pos_max) {
    grd->scr_pos = grd->scr_pos_max;
  }
  else {
    grd->scr_pos = scr_pos;
  }
}
///
///freeGradient(gradient)
/******************************************************************************
 * Deallocates the gradient struct created by createGradient().               *
 * NOTE: Will deallocate the colortable if one is attached to it.             *
 ******************************************************************************/
VOID freeGradient(struct Gradient* grd)
{
  if (grd) {
    if (grd->color_table) freeColorTable(grd->color_table);
    FreeMem(grd->colors, grd->num_colors * grd->type * sizeof(UWORD));
    FreeMem(grd, sizeof(struct Gradient));
  }
}
///

///newRainbow(num_ops, op_size, num_insts, blitable, num_end_insts)
/******************************************************************************
 * num_ops : How many copper operations we want (excluding the end CopOp).    *
 * op_size : How many instruction each operation contains. When given every   *
 *      CopOp.pointer will be set here. Otherwise you have to set each one    *
 *      individually. Because omiting (set to 0) will imply that you want     *
 *      CopOps with varying op_sizes.                                         *
 * num_insts : total number of instructions in all of the copOps (excluding   *
 *      the end instruction). This has to be given manually if the ops are of *
 *      varying instruction sizes.                                            *
 * blitable : means the gradients used by this rainbow are scrollable.        *
 *      blitable allows gradient blits into the copperlist of this rainbow!   *
 * num_end_insts : is the number of instructions planned to be in the         *
 *      terminating end CopOp.                                                *
 ******************************************************************************/
struct Rainbow* newRainbow(UWORD num_ops, UWORD op_size, UWORD num_insts, BOOL blitable, UWORD num_end_insts)
{
  struct Rainbow* rb = AllocMem(sizeof(struct Rainbow) + (num_ops + 1) * sizeof(struct CopOp), MEMF_ANY | MEMF_CLEAR);

  if (rb) {
    rb->num_ops = num_ops;
    rb->blitable = blitable;
    if (!num_insts) num_insts = num_ops * op_size;
    if (!num_end_insts) num_end_insts = 1;
    rb->num_insts = num_insts + num_end_insts;
    rb->list = AllocMem(rb->num_insts * 4, MEMF_CHIP);
    if (rb->list) {
      ULONG i;
      for (i = 0; i < rb->num_insts; i++) {
        rb->list[i] = NOOP;
      }
    }
    else {
      FreeMem(rb, sizeof(struct Rainbow) + (num_ops + 1) * sizeof(struct CopOp));
      return NULL;
    }
  }

  if (op_size) {
    ULONG i;
    for (i = 0; i <= num_ops; i++) {
      rb->copOps[i].size = op_size;
      rb->copOps[i].pointer = rb->list + (i * op_size);
    }
  }

  return rb;
}
///
///createEmptyRainbow()
/******************************************************************************
 * Creates a dummy rainbow with no color instructions but only with an        *
 * instruction to wait for the end of line 255. The DYNAMIC_COPPERLIST        *
 * algorithm requires at least this to operate.                               *
 ******************************************************************************/
struct Rainbow* createEmptyRainbow()
{
#if SCREEN_END < 256
  struct Rainbow* rb = newRainbow(0, 0, 0, FALSE, 1);
  if (rb) {
    ULONG copperList[1] = {
      END
    };

    CopyMem(copperList, rb->list, 4);

    rb->copOps[0].wait = 0xFFFF;
    rb->copOps[0].size = 1;
    rb->copOps[0].pointer = rb->list;
  }
#else
  struct Rainbow* rb = newRainbow(1, 1, 0, FALSE, 1);
  if (rb) {
    ULONG copperList[2] = {
      WAIT(225, 255),  // Wait for the end of raster line 255 (Hardware Coord)
      END
    };

    CopyMem(copperList, rb->list, 8);

    rb->copOps[0].wait = 256;
    rb->copOps[0].size = 1;
    rb->copOps[0].pointer = rb->list;
    rb->copOps[1].wait = 0xFFFF;
    rb->copOps[1].size = 1;
    rb->copOps[1].pointer = rb->list + 1;
  }
#endif

  return rb;
}
///
///fillCorrespondanceTable(rainbow)
/******************************************************************************
 * Them member "correspondance" on struct Rainbow is an array that maps each  *
 * raster line (in screen coordinate, NOT hardware coordinate) to the CopOp   *
 * index that waits it. This prevents a lookup on the rainbow when the number *
 * of CopOps are less then SCREEN_HEIGHT (meaning there isn't a CopUp present *
 * for each rasterline, which means we cannot get the CopOp by index).        *
 * This function fills this table on a rainbow after its wait values are set. *
 ******************************************************************************/
VOID fillCorrespondanceTable(struct Rainbow* rb)
{
  ULONG y = rb->copOps[0].wait - SCREEN_START; // iterator for rasterline (screen coordinate!)
  ULONG c;                                     // iterator for copOp index

  for (c = 0; c < rb->num_ops; c++) {
    for (; y < rb->copOps[c].wait - SCREEN_START; y++) {
      rb->correspondance[y] = c - 1;
    }
    rb->correspondance[y++] = c;
  }
  for (; y < SCREEN_HEIGHT; y++) {
    rb->correspondance[y] = c - 1;
  }
}
///
///createRainbow(gradientsList, end_insts)
/******************************************************************************
 * Creates the rainbow CopOps from the given list of gradients (which is a    *
 * NULL terminated array of gradient pointers). Can only be used with         *
 * DYNAMIC_COPPERLIST. This function can create structurally different CopOp  *
 * lists depending on the properties of the gradients passed in to not waste  *
 * valuable Copper DMA time. It does this by analyzing the given gradients in *
 * many iterations, so it should NOT be called during gameplay loop. Only     *
 * call it at level init phase.                                               *
 * The array of end instructions that will terminate the copperlist to be     *
 * created should be passed in end_insts. It doesn't need to be a single END  *
 * instruction. i.e you can set a bottom of screen dash board in this array   *
 * but still HAS TO BE terminated with an END instruction always! If ommited, *
 * the created CopOps list and so the copperlist it points to will be         *
 * terminated with a single END instruction.                                  *
 ******************************************************************************/
struct Rainbow* createRainbow(struct Gradient** gradList, ULONG* end_insts)
{
  struct Rainbow* rb = NULL;
  struct Gradient* gradList_s[8] = {0}; //WARNING: This limits num_grads to 8 (which is already too much)!
  BOOL has_movable = FALSE;
  BOOL has_scrollable = FALSE;
  ULONG num_grads = 0;
  ULONG num_end_insts = 1;
  ULONG default_end[1] = {END};
  ULONG i;

  // Pass1: get the size of the gradList
  if (gradList) {
    struct Gradient** grd = gradList;

    while (*grd) {
      grd++;
      num_grads++;
    }
  }

  // Pass2: get the size of end_insts
  if (end_insts) {
    ULONG* inst = end_insts;

    while (*inst != END) {
      inst++;
      num_end_insts++;
    }
  }
  else end_insts = default_end;

  // If no gradients passed in create an empty rainbow
  if (!num_grads) {
    if (!end_insts) {
      return createEmptyRainbow();
    }
    else {
#if SCREEN_END < 256
      rb = newRainbow(0, 0, 0, FALSE, num_end_insts);
      if (rb) {
        CopyMem(end_insts, rb->list, num_end_insts * sizeof(CLINST));

        rb->copOps[0].wait = 0xFFFF;
        rb->copOps[0].size = num_end_insts;
        rb->copOps[0].pointer = rb->list;
#else
      rb = newRainbow(1, 0, 1, FALSE, num_end_insts);
      if (rb) {
        *rb->list = WAIT(225, 255);
        CopyMem(end_insts, rb->list + 1, num_end_insts * sizeof(CLINST));

        rb->copOps[0].wait = 256;
        rb->copOps[0].size = 1;
        rb->copOps[0].pointer = rb->list;
        rb->copOps[1].wait = 0xFFFF;
        rb->copOps[1].size = num_end_insts;
        rb->copOps[1].pointer = rb->list + 1;
#endif
        return rb;
      }
      else return NULL;
    }
  }

  // Pass3: sort the gradList passed in
  {
    int i, j;
    struct Gradient* key;

    CopyMem(gradList, gradList_s, num_grads * sizeof(struct Gradlist*));

    for (i = 1; i < num_grads; i++) {
      key = gradList_s[i];
      j = i - 1;

      while (j >= 0 && gradList_s[j]->scr_pos > key->scr_pos) {
        gradList_s[j + 1] = gradList_s[j];
        j = j - 1;
      }
      gradList_s[j + 1] = key;
    }
  }

  // Pass4: Check if there are any movable gradients
  for (i = 0; i < num_grads; i++) {
    if (gradList_s[i]->movable) {
      has_movable = TRUE;
      break;
    }
  }

  if (has_movable) {
    UWORD num_ops = SCREEN_HEIGHT;
    UWORD op_size = 1;   // Every CopOp has at least the WAIT instruction.
    BOOL has_AGA = FALSE;
    UBYTE num_AGA_Grads = 0;

    has_scrollable = TRUE;

    for (i = 0; i < num_grads; i++) {
      // AGA gradients require 2 instructions to set a color reg.
      if (gradList_s[i]->type == GRD_TYPE_AGA) { op_size += 2; has_AGA = TRUE; }
      else op_size++;
    }

    // If there is at least one AGA gradient 2 more instructions will be
    // needed to set LOCT on and off
    if (has_AGA) op_size += 2;

    // Pass5: Set required values on gradients
    for (i = 0; i < num_grads; i++) {
      gradList_s[i]->blitable = TRUE; // One movable gradient casts all gradients blitable!
      gradList_s[i]->modulus = op_size * 4 - 2;
      gradList_s[i]->index = i + 1;
      if (has_AGA) {
        if (gradList_s[i]->type == GRD_TYPE_AGA) {
          gradList_s[i]->loct_offs = (num_grads + 1 + num_AGA_Grads - i) * 2;
          num_AGA_Grads++;
        }
      }
    }

    rb = newRainbow(num_ops, op_size, 0, has_scrollable, num_end_insts);
    rb->gradList = gradList;

    // Pass6: Set essential copper instructions on CopOps
    if (rb) {
      ULONG c, g, i;
      for (c = 0; c < num_ops; c++) {
        ULONG wait = c + SCREEN_START;
        ULONG copper_wait;

        if (wait < 256)
          copper_wait = WAIT(0, wait);
        else if (wait == 256)
          copper_wait = WAIT(225, 255);
        else
          copper_wait = WAIT(0, wait - 256);

        rb->copOps[c].wait = wait;
        rb->copOps[c].pointer[0] = copper_wait;
        for (g = 0, i = 1; g < num_grads; g++, i++) {
          rb->copOps[c].pointer[i] = MOVE(COLOR00 + (gradList_s[g]->col_reg * 2), 0x0000);
        }
        if (has_AGA) {
          rb->copOps[c].pointer[i] = MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT);
          for (g = 0, ++i; g < num_grads; g++) {
            if (gradList_s[g]->type == GRD_TYPE_AGA) {
              rb->copOps[c].pointer[i] = MOVE(COLOR00 + (gradList_s[g]->col_reg * 2), 0x0000);
              i++;
            }
          }
          rb->copOps[c].pointer[i] = MOVE(BPLCON3, BPLCON3_V);
        }
      }

      // Set the end CopOp
      rb->copOps[c].wait = 0xFFFF;
      rb->copOps[c].size = num_end_insts;
      CopyMem(end_insts, rb->copOps[c].pointer, num_end_insts * 4);

      // Pass7: Fill correspondance
      fillCorrespondanceTable(rb);

      // Pass8: Blit in color values from the gradients into the rainbow
      updateRainbow(rb);
    }
  }
  else { // no movable gradients
    UWORD num_ops = 0;
    UWORD num_insts = 0;
    UBYTE tbl_inst[SCREEN_HEIGHT] = {0};
    UBYTE tbl_aga[SCREEN_HEIGHT] = {0};
    BOOL scrollable[SCREEN_HEIGHT] = {FALSE};
    ULONG i, g, l;

    // Pass 5: Mark rasterlines for scrollable gradients
    for (g = 0; g < num_grads; g++) {
      if (gradList_s[g]->scrollable) has_scrollable = TRUE;

      for (i = gradList_s[g]->scr_pos; i < gradList_s[g]->scr_pos + gradList_s[g]->vis; i++) {
        if (gradList_s[g]->scrollable) {
          scrollable[i] = TRUE;
        }
      }
    }

    // Pass 6: Enlarge rasterlines marked as scrollable to colliding non-scrollable gradients
    for (g = 0; g < num_grads ; g++) {
      if (!gradList_s[g]->scrollable) {
        BOOL has_scrollable = FALSE;
        for (i = gradList_s[g]->scr_pos; i < gradList_s[g]->scr_pos + gradList_s[g]->vis; i++) {
          if (scrollable[i]) {
            has_scrollable = TRUE;
            break;
          }
        }
        if (has_scrollable) {
          for (i = gradList_s[g]->scr_pos; i < gradList_s[g]->scr_pos + gradList_s[g]->vis; i++) {
            scrollable[i] = TRUE;
          }
        }
      }
    }

    // Pass 7: Calculate every scanline’s instruction requirements
    for (i = 0; i < SCREEN_HEIGHT; i++) {
      for (g = 0; g < num_grads; g++) {
        UBYTE index = 1;

        if (i >= gradList_s[g]->scr_pos && i < gradList_s[g]->scr_pos + gradList_s[g]->vis) {

          // Allocate an index for the gradient
          if (has_scrollable && gradList_s[g]->index == 0) {
            UBYTE num_grads_so_far;
            UBYTE num_AGA_Grads_so_far;

            try_again_index:
            num_grads_so_far = 0;
            num_AGA_Grads_so_far = 0;

            for (l = 0; l < num_grads; l++) {
              if (i >= gradList_s[l]->scr_pos && i < gradList_s[l]->scr_pos + gradList_s[l]->vis) {
                num_grads_so_far++;

                if (gradList_s[l]->index == index) {
                  if (gradList_s[l]->type == GRD_TYPE_AGA) {
                    gradList_s[l]->loct_offs += 2;
                    num_AGA_Grads_so_far++;
                  }
                  index++;
                  goto try_again_index;
                }
              }
            }
            gradList_s[g]->index = index;
            gradList_s[g]->blitable = TRUE;
            if (gradList_s[g]->type == GRD_TYPE_AGA) {
              gradList_s[g]->loct_offs = (num_grads_so_far + 1 + num_AGA_Grads_so_far - (index - 1)) * 2;
            }
          }

          if (tbl_inst[i]) {
            if (gradList_s[g]->type == GRD_TYPE_AGA) {
              tbl_inst[i] += 2;
              tbl_aga[i]++;
            }
            else tbl_inst[i]++;
          }
          else {
            if (gradList_s[g]->type == GRD_TYPE_AGA) {
              tbl_inst[i] = 5;
              tbl_aga[i]++;
            }
            else tbl_inst[i] = 2;
          }
        }
      }
    }

    // Pass 8: Set every scrollable region to have a single op_size
    for (g = 0; g < num_grads; g++) {
      UBYTE max_op_size = 0;
      UBYTE max_aga = 0;

      if (scrollable[gradList_s[g]->scr_pos]) {
        for (i = gradList_s[g]->scr_pos; i < gradList_s[g]->scr_pos + gradList_s[g]->vis; i++) {
          if (tbl_inst[i] > max_op_size) max_op_size = tbl_inst[i];
          if (tbl_aga[i] > max_aga) max_aga = tbl_aga[i];
        }
        for (i = gradList_s[g]->scr_pos; i < gradList_s[g]->scr_pos + gradList_s[g]->vis; i++) {
          tbl_inst[i] = max_op_size;
          tbl_aga[i] = max_aga;
        }
      }
    }

    // A CopOp for the wait for hardware coordinate 256 is compulsory!
#if SCREEN_END >= 256
      if (!tbl_inst[256 - SCREEN_START]) {
        tbl_inst[256 - SCREEN_START] = 1;
      }
#endif

    // Pass 9: Count num_ops and num_insts
    for (i = 0; i < SCREEN_HEIGHT; i++) {
      if (tbl_inst[i]) {
        num_ops++;
        num_insts += tbl_inst[i];
      }
    }

    // Pass 10: Set gradient modulus values
    for (g = 0; g < num_grads; g++) {
      if (gradList_s[g]->blitable) {
        gradList_s[g]->modulus = tbl_inst[gradList_s[g]->scr_pos] * 4 - 2;
      }
    }

    rb = newRainbow(num_ops, 0, num_insts, has_scrollable, num_end_insts);
    rb->gradList = gradList;

    if (rb) {
      ULONG c, g, i, y;
      ULONG* ptr = rb->list;

      // Pass 11: Fill in the instructions
      if (has_scrollable) {
        for (y = 0, c = 0; y < SCREEN_HEIGHT; y++) {
          if (tbl_inst[y]) {
            BOOL has_AGA = FALSE;
            ULONG wait = y + SCREEN_START;
            ULONG copper_wait;

            if (wait < 256)
              copper_wait = WAIT(0, wait);
            else if (wait == 256)
              copper_wait = WAIT(225, 255);
            else
              copper_wait = WAIT(0, wait - 256);

            rb->copOps[c].wait = wait;
            rb->copOps[c].size = tbl_inst[y];
            rb->copOps[c].pointer = ptr;

            rb->copOps[c].pointer[0] = copper_wait;

            i = 1;
            while (i <= 8) {
              for (g = 0; g < num_grads; g++) {
                if (y >= gradList_s[g]->scr_pos && y < gradList_s[g]->scr_pos + gradList_s[g]->vis && gradList_s[g]->index == i) {
                  break;
                }
              }
              if (g < num_grads) {
                rb->copOps[c].pointer[i] = MOVE(COLOR00 + (gradList_s[g]->col_reg * 2), 0x0000);
                if (gradList_s[g]->type == GRD_TYPE_AGA) {
                  has_AGA = TRUE;
                  rb->copOps[c].pointer[i + gradList_s[g]->loct_offs / 2] = MOVE(COLOR00 + (gradList_s[g]->col_reg * 2), 0x0000);
                }
              }
              i++;
            }
            if (has_AGA) {
              rb->copOps[c].pointer[tbl_inst[y] - tbl_aga[y] - 2] = MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT);
              rb->copOps[c].pointer[tbl_inst[y] - 1] = MOVE(BPLCON3, BPLCON3_V);
            }

            c++;
            ptr += tbl_inst[y];
          }
        }

        // Pass12: Fill correspondance
        fillCorrespondanceTable(rb);
        // Pass13: Fill copperlist with values
        updateRainbow(rb);
      }
      else { // does not have scrollable
        for (y = 0, c = 0; y < SCREEN_HEIGHT; y++) {
          if (tbl_inst[y]) {
            BOOL has_AGA = FALSE;
            ULONG wait = y + SCREEN_START;
            ULONG copper_wait;

            if (wait < 256)
              copper_wait = WAIT(0, wait);
            else if (wait == 256)
              copper_wait = WAIT(225, 255);
            else
              copper_wait = WAIT(0, wait - 256);

            rb->copOps[c].wait = wait;
            rb->copOps[c].size = tbl_inst[y];
            rb->copOps[c].pointer = ptr;

            rb->copOps[c].pointer[0] = copper_wait;

            i = 1;
            for (g = 0; g < num_grads; g++) {
              if (y >= gradList_s[g]->scr_pos && y < gradList_s[g]->scr_pos + gradList_s[g]->vis) {
                if (gradList_s[g]->type == GRD_TYPE_AGA) has_AGA = TRUE;
                // WARNING: Because this rainbow doesn't have any scrollable gradients, we utilize member "pos" for another purpose!
                rb->copOps[c].pointer[i] = MOVE(COLOR00 + (gradList_s[g]->col_reg * 2), gradList_s[g]->colors[gradList_s[g]->pos++]);
                i++;
              }
            }
            if (has_AGA) {
              rb->copOps[c].pointer[i] = MOVE(BPLCON3, BPLCON3_V | BPLCON3_LOCT);
              i++;
              for (g = 0; g < num_grads; g++) {
                if (gradList_s[g]->type == GRD_TYPE_AGA && y >= gradList_s[g]->scr_pos && y < gradList_s[g]->scr_pos + gradList_s[g]->vis) {
                  rb->copOps[c].pointer[i] = MOVE(COLOR00 + (gradList_s[g]->col_reg * 2), gradList_s[g]->colors[gradList_s[g]->pos++]);
                  i++;
                }
              }
              rb->copOps[c].pointer[i] = MOVE(BPLCON3, BPLCON3_V);
            }

            c++;
            ptr += tbl_inst[y];
          }
        }

        // Pass12: Fill correspondance
        fillCorrespondanceTable(rb);
      }

      // Set the end CopOp
      rb->copOps[c].wait = 0xFFFF;
      rb->copOps[c].size = num_end_insts;
      rb->copOps[c].pointer = ptr;

      CopyMem(end_insts, ptr, num_end_insts * 4);
    }
  }

  return rb;
}
///
///freeRainbow(rainbow)
/******************************************************************************
 * Deallocates the rainbow created by newRainbow(), createRainbow() or        *
 * createEmptyRainbow().                                                      *
 * NOTE: Does NOT free the gradients set on its gradList.                     *
 ******************************************************************************/
VOID freeRainbow(struct Rainbow* rb)
{
  if (rb) {
    FreeMem(rb->list, rb->num_insts * 4);
    FreeMem(rb, sizeof(struct Rainbow) + (rb->num_ops + 1) * sizeof(struct CopOp));
  }
}
///

///initGradientBlit()
VOID initGradientBlit()
{
  busyWaitBlit();
  custom.bltcon0 = 0x9F0; // use A and D. Op: D = A (direct copy)
  custom.bltcon1 = 0;
  custom.bltafwm = 0xFFFF;
  custom.bltalwm = 0xFFFF;
}
///
///blitGradient(gradient, rainbow)
/******************************************************************************
 * TODO: 1) calculate bltapt according to scroll values! (DONE)               *
 *       2) cover out of boundary (vis) scrolls!                              *
 *       3) cover moved gradient blits!     DONE                              *
 *       4) Fill a proper explanation here!                                   *
 *       5) Implement the distinction between scrollable and blitable blits   *
 ******************************************************************************/
VOID blitGradient(struct Gradient* grd, struct Rainbow* rb)
{
  if (grd->blitable) {
    UWORD* bltdpt  = (UWORD*)(rb->copOps[rb->correspondance[grd->scr_pos]].pointer + grd->index) + 1;

    if (grd->type == GRD_TYPE_AGA) {
      UWORD* bltapt  = grd->colors + grd->pos * 2;

      custom.bltamod = 2;
      custom.bltdmod = grd->modulus;
      custom.bltapt  = bltapt;
      custom.bltdpt  = bltdpt;

      custom.bltsize = (grd->vis << 6) | 1;

      busyWaitBlit();
      custom.bltapt  = bltapt + 1;
      custom.bltdpt  = bltdpt + grd->loct_offs;

      custom.bltsize = (grd->vis << 6) | 1;
    }
    else {
      custom.bltamod = 0;
      custom.bltdmod = grd->modulus;
      custom.bltapt  = grd->colors + grd->pos;
      custom.bltdpt  = bltdpt;

      custom.bltsize = (grd->vis << 6) | 1;
    }
  }
}
///
///updateRainbow(rainbow)
/******************************************************************************
 * Updates the latests states of the gradients on a rainbow to itself by      *
 * blitting the new instruction values in case of a move or scroll.           *
 ******************************************************************************/
VOID updateRainbow(struct Rainbow* rb)
{
  struct Gradient** grd_ptr = rb->gradList;
  struct Gradient* grd;

  initGradientBlit();

  while ((grd = *grd_ptr)) {
    blitGradient(grd, rb);

    grd_ptr++;
  }
}
///
