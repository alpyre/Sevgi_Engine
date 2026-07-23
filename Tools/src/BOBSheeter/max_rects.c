/******************************************************************************
 * MaxRects implementation for Sevgi Engine BOBSheets                         *
 * Configure heuristics depth using the defines: MAX_RECTS, OPT_PASSES        *
 * WARNING: Mutates widths (rounds up to the nearest multiple of 16) on the   *
 * input ImageDesc array passed! Use the widths from the source table while   *
 * saving the sheet.                                                          *
 ******************************************************************************/

///includes
#include <exec/types.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>

#include "max_rects.h"
///
///defines
#define MAX_RECTS  1024
#define OPT_PASSES 32

#define ROUND_TO_16(a) ((a + 15) & 0xFFFFFFF0)
///
///structs
struct Rect {
  UWORD x, y, w, h;
};
///
///globals
STATIC ULONG random_seed = 1234567;
STATIC struct Rect freeRects[MAX_RECTS];
STATIC LONG freeCount;
///
///protos
STATIC ULONG rangeRand(ULONG range);
STATIC VOID shuffle(struct ImageDesc* arr, ULONG count);
STATIC VOID sortByHeight(struct ImageDesc* arr, ULONG count);
STATIC VOID sortByArea(struct ImageDesc* arr, ULONG count);
///

/******************************************************************************
 * Utility functions                                                          *
 ******************************************************************************/
///rangeRand(ULONG range)
/******************************************************************************
 * Creates a pseudorandom ULONG number within the given range.                *
 ******************************************************************************/
STATIC ULONG rangeRand(ULONG range)
{
  random_seed = FastRand(random_seed);
  return random_seed % range;
}
///
///shuffle(arr, count)
/******************************************************************************
 * Randomly shuffles the array.                                               *
 ******************************************************************************/
STATIC VOID shuffle(struct ImageDesc* arr, ULONG count)
{
  ULONG i;

  for (i = count - 1; i > 1; i--) {
    ULONG j = rangeRand(i);
    struct ImageDesc temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
}
///
///sortByHeight(arr, count)
/******************************************************************************
 * Sorts the array by image height using selection sort.                      *
 ******************************************************************************/
STATIC VOID sortByHeight(struct ImageDesc* arr, ULONG count)
{
  ULONG i;
  ULONG j;

  for (i = 0; i < count - 1; i++)
    for (j = i + 1; j < count; j++)
      if (arr[j].height > arr[i].height) {
        struct ImageDesc temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
}
///
///sortByArea(arr, count)
/******************************************************************************
 * Sorts the array by image rectangle area using selection sort.              *
 ******************************************************************************/
STATIC VOID sortByArea(struct ImageDesc* arr, ULONG count)
{
  ULONG i;
  ULONG j;

  for (i = 0; i < count - 1; i++)
    for (j = i + 1; j < count; j++) {
      ULONG area_i = (ULONG)arr[i].width * arr[i].height;
      ULONG area_j = (ULONG)arr[j].width * arr[j].height;
      if (area_j > area_i) {
        struct ImageDesc temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
}
///

/******************************************************************************
 * MaxRects Core                                                              *
 ******************************************************************************/
///ResetFreeRects(w, h)
STATIC VOID ResetFreeRects(UWORD w, UWORD h)
{
  freeRects[0].x = 0;
  freeRects[0].y = 0;
  freeRects[0].w = w;
  freeRects[0].h = h;
  freeCount = 1;
}
///
///PruneFreeRects()
STATIC VOID PruneFreeRects(VOID)
{
  LONG i;
  LONG j;

  for (i = 0; i < freeCount; i++)
    for (j = i + 1; j < freeCount; j++) {
      struct Rect a = freeRects[i];
      struct Rect b = freeRects[j];

      if (a.x >= b.x && a.y >= b.y &&
          a.x + a.w <= b.x + b.w &&
          a.y + a.h <= b.y + b.h) {
        freeRects[i] = freeRects[--freeCount];
        i--;
        break;
      }

      if (b.x >= a.x && b.y >= a.y &&
          b.x + b.w <= a.x + a.w &&
          b.y + b.h <= a.y + a.h) {
        freeRects[j] = freeRects[--freeCount];
        j--;
      }
    }
}
///
///FindPosition(best, w, h)
STATIC ULONG FindPosition(struct Rect *best, UWORD w, UWORD h)
{
  ULONG bestArea = 0xFFFFFFFF;
  ULONG found = 0;
  ULONG i;

  for (i = 0; i < freeCount; i++) {
    if (w <= freeRects[i].w && h <= freeRects[i].h) {
      ULONG area = freeRects[i].w * freeRects[i].h - w * h;

      if (area < bestArea) {
        bestArea = area;
        best->x = freeRects[i].x;
        best->y = freeRects[i].y;
        best->w = w;
        best->h = h;
        found = 1;
      }
    }
  }

  return found;
}
///
///PackMaxRects(in, out, count, sheet_w)
STATIC UWORD PackMaxRects(struct ImageDesc *in, struct ImageDesc *out, int count, ULONG sheet_w)
{
  ULONG max_h = 0;
  ULONG i;

  ResetFreeRects(sheet_w, 65535);

  for (i = 0; i < count; i++) {
    ULONG j;
    struct Rect node;
    UWORD w = in[i].width;
    UWORD h = in[i].height;

    if (!FindPosition(&node, w, h))
      return 0xFFFF;

    // Sheet height owerflow check
    if ((ULONG)h + node.y >= 65535)
      return 0xFFFF;

    // freeCount owerflow check
    if (freeCount >= MAX_RECTS - 4)
      return 0xFFFF;

    // Split rects
    for (j = 0; j < freeCount; j++) {
      struct Rect r = freeRects[j];

      if (node.x >= r.x + r.w || node.x + node.w <= r.x ||
          node.y >= r.y + r.h || node.y + node.h <= r.y)
        continue;

      if (node.x > r.x) {
        freeRects[freeCount++] = (struct Rect){r.x, r.y, node.x - r.x, r.h};
      }

      if (node.x + node.w < r.x + r.w) {
        freeRects[freeCount++] = (struct Rect){node.x + node.w, r.y, (r.x + r.w) - (node.x + node.w), r.h};
      }

      if (node.y > r.y) {
        freeRects[freeCount++] = (struct Rect){r.x, r.y, r.w, node.y - r.y};
      }

      if (node.y + node.h < r.y + r.h) {
        freeRects[freeCount++] = (struct Rect){r.x, node.y + node.h, r.w, (r.y + r.h) - (node.y + node.h)};
      }

      // Remove original
      freeRects[j] = freeRects[--freeCount];
    }

    PruneFreeRects();

    out[i] = in[i];
    out[i].x_pos = node.x;
    out[i].y_pos = node.y;

    if (node.y + h > max_h)
      max_h = node.y + h;
  }

  return max_h;
}
///

/******************************************************************************
 * Exported interface                                                         *
 ******************************************************************************/
struct ImageDesc* PackImagesMaxRects(struct ImageDesc* input, ULONG count, UWORD* out_w, UWORD* out_h)
{
  struct ImageDesc* work = AllocMem(sizeof(struct ImageDesc) * count, MEMF_ANY);
  if (work) {
    struct ImageDesc* temp = AllocMem(sizeof(struct ImageDesc) * count * 2, MEMF_ANY);
    if (temp) {
      struct ImageDesc* best = temp + count;

      ULONG best_area = 0xFFFFFFFF;
      UWORD best_w = 0;
      UWORD best_h = 0;

      ULONG total_area = 0;
      ULONG max_sheet_w = 0;
      UWORD max_w = 0;
      UWORD max_h = 0;
      ULONG i;
      ULONG pass;

      // Initialize input array
      for (i = 0; i < count; i++) {
        input[i].original_index = i;                          // Remember initial order
        input[i].width = ROUND_TO_16(input[i].width);         // Precalculate rounded widths
        total_area += input[i].width * input[i].height;       // Calculate ideal sheet area
        if (input[i].width > max_w) max_w = input[i].width;   // Find widest image width
        if (input[i].height > max_h) max_h = input[i].height; // Find tallest image height
      }

      // Just an approximation
      max_sheet_w = total_area / max_h;

      for (pass = 0; pass < OPT_PASSES; pass++) {
        UWORD w;

        CopyMem(input, work, sizeof(struct ImageDesc) * count);

        switch (pass) {
          case 0:
            sortByHeight(work, count);
          break;
          case OPT_PASSES / 2:
            sortByArea(work, count);
          break;
          default:
            shuffle(work, count);
          break;
        }

        for (w = max_w; w <= max_sheet_w; w += 16) {
          ULONG area;
          UWORD h = PackMaxRects(work, temp, count, w);
          if (h == 0xFFFF) continue;

          area = (ULONG)w * h;

          if (area < best_area) {
            best_area = area;
            best_w = w;
            best_h = h;

            // Take a copy of the best arrangement so far
            CopyMem(temp, best, sizeof(struct ImageDesc) * count);
          }
        }
      }

      // Restore order
      for (i = 0; i < count; i++)
        work[best[i].original_index] = best[i];

      *out_w = best_w;
      *out_h = best_h;

      FreeMem(temp, sizeof(struct ImageDesc) * count * 2);
    }
    else {
      FreeMem(work, sizeof(struct ImageDesc) * count);
      work = NULL;
    }
  }

  return work;
}
