///includes
#include <stdio.h>
#include <string.h>

#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/screens.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <graphics/display.h>
#include <graphics/layers.h>
#include <graphics/clip.h>
#include <graphics/gfxmacros.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <clib/alib_protos.h>

#include "system.h"
#include "display.h"
#include "fonts.h"
#include "ui.h"
///
///defines
#if defined UI_USE_SCROLLBARS && !defined UI_USE_SCROLLING
  #define UI_USE_SCROLLING
#endif

#if defined UI_USE_INTEGER_GADGETS && !defined UI_USE_STRING_GADGETS
  #define UI_USE_STRING_GADGETS
#endif

#define IS_IN_RECT(x, y, x1, y1, x2, y2) ((x) >= (x1) && (x) < (x2) && (y) >= (y1) && (y) < (y2))
///
///prototypes
VOID setParents(struct UIO_Group* root);
VOID setSizes(struct UIO_Group* root);
VOID applyInheritance(struct UIO_Group* root);

struct Region* inheritClipRegion(struct UIObject* parent);
VOID setClip(struct UIObject* obj);
VOID removeClip(VOID);

struct UIObject* findPointedObject(struct UIO_Group* root, WORD pointer_x, WORD pointer_y, BOOL* disabled);
VOID makeObjectVisible(struct UIObject* object);

VOID drawFrame(WORD x1, WORD y1, WORD x2, WORD y2, ULONG style);
///
///globals
STATIC ULONG ui_screen_start = NULL;
STATIC struct RastPort* ui_rastport = NULL;  // the layered RastPort to draw ui graphics
STATIC struct UIObject* ui_current_clipper = NULL; // last object setClip() has been called with
STATIC struct UIObject* ui_hovered_object = NULL; // the object under the mouse pointer
STATIC struct UIObject* ui_active_object = NULL; // the object in focus
STATIC struct UIObject** ui_cycle_chain = NULL; // a NULL terminated array of UIObject pointers
STATIC ULONG ui_cycle_chain_size = 0;
STATIC ULONG ui_cycle_chain_index; // the index of currently selected object in cycle chain
STATIC struct UIObject* ui_selected_object = NULL; // the object in focus
STATIC struct MinList ui_anim_list;
STATIC UWORD ui_disabled_pattern1[] = {0xAAAA, 0x5555};
STATIC UWORD ui_disabled_pattern2[] = {0x5555, 0xAAAA};
#ifdef UI_USE_STRING_GADGETS
STATIC UBYTE ui_password_string[] = {UIOV_STRING_PASSWORD_CHAR, NULL};
STATIC UBYTE ui_dotted_string[] = {UIOV_STRING_DOTTED_CHAR, NULL};
#endif // UI_USE_STRING_GADGETS
///

///inheritXOffset(parent)
/******************************************************************************
 * Traverses the UI tree backwards starting from the given group object       *
 * recursively to gather the horizontal offset caused by all of the parent    *
 * group's scroll states and placements on the rastport.                      *
 ******************************************************************************/
WORD inheritXOffset(struct UIObject* parent)
{
  struct UIO_Group* parent_group = (struct UIO_Group*) parent;
  WORD offset = parent->x + parent_group->margin.left;

  if (parent->parent) offset += inheritXOffset(parent->parent);

  return (WORD)(offset - parent_group->scroll.x);
}
///
///inheritYOffset(parent)
/******************************************************************************
 * Traverses the UI tree backwards starting from the given group object       *
 * recursively to gather the vertical offset caused by all of the parent      *
 * group's scroll states and placements on the rastport.                      *
 ******************************************************************************/
WORD inheritYOffset(struct UIObject* parent)
{
  struct UIO_Group* parent_group = (struct UIO_Group*) parent;
  WORD offset = parent->y + parent_group->margin.top;

  if (parent->parent) offset += inheritYOffset(parent->parent);

  return (WORD)(offset - parent_group->scroll.y);
}
///
///inheritX(obj)
/******************************************************************************
 * Returns the actual horizontal position of an UIObject on the rastport      *
 * regarding the offsets caused by the parent group object(s) that contain it.*
 ******************************************************************************/
WORD inheritX(struct UIObject* obj)
{
  WORD x = obj->x;

  if (obj->parent) x += inheritXOffset(obj->parent);

  return x;
}
///
///inheritY(obj)
/******************************************************************************
 * Returns the actual vertical position of an UIObject on the rastport        *
 * regarding the offsets caused by the parent group object(s) that contain it.*
 ******************************************************************************/
WORD inheritY(struct UIObject* obj)
{
  WORD y = obj->y;

  if (obj->parent) y += inheritYOffset(obj->parent);

  return y;
}
///

///inheritClipRegion(parent)
/******************************************************************************
 * Creates a clipping region for a given group object regarding the clipping  *
 * caused by it's parent (containing) group object(s).                        *
 ******************************************************************************/
struct Region* inheritClipRegion(struct UIObject* parent)
{
  struct UIO_Group* parent_group = (struct UIO_Group*) parent;
  struct Region* region = NULL;
  struct Rectangle rect;
  WORD x = parent->x;
  WORD y = parent->y;

  if (parent->parent) {
    x += inheritXOffset(parent->parent);
    y += inheritYOffset(parent->parent);
    region = inheritClipRegion(parent->parent);

    rect.MinX = x + parent_group->margin.left;
    rect.MinY = y + parent_group->margin.top;
#ifdef UI_USE_TABBED_GROUPS
    if (parent_group->flags & UIOF_TABBED) {
      rect.MaxX = x + parent_group->width - parent_group->margin.right;
      rect.MaxY = y + parent_group->height - parent_group->margin.bottom;
    }
    else {
#endif // UI_USE_TABBED_GROUPS
#ifdef UI_USE_SCROLLBARS
      rect.MaxX = x + parent_group->width - parent_group->margin.right - (parent_group->scroll.bar_vert ? UIOV_SCROLLBAR_WIDTH + parent_group->child_gap : 0);
      rect.MaxY = y + parent_group->height - parent_group->margin.bottom - (parent_group->scroll.bar_horiz ? UIOV_SCROLLBAR_HEIGHT + parent_group->child_gap : 0);
#else // UI_USE_SCROLLBARS
      rect.MaxX = x + parent_group->width - parent_group->margin.right;
      rect.MaxY = y + parent_group->height - parent_group->margin.bottom;
#endif // UI_USE_SCROLLBARS
#ifdef UI_USE_TABBED_GROUPS
    }
#endif // UI_USE_TABBED_GROUPS

    AndRectRegion(region, &rect);
  }
  else {
    region = NewRegion();

    rect.MinX = x + parent_group->margin.left;
    rect.MinY = y + parent_group->margin.top;
#ifdef UI_USE_TABBED_GROUPS
    if (parent_group->flags & UIOF_TABBED) {
      rect.MaxX = x + parent_group->width - parent_group->margin.right;
      rect.MaxY = y + parent_group->height - parent_group->margin.bottom;
    }
    else {
#endif // UI_USE_TABBED_GROUPS
#ifdef UI_USE_SCROLLBARS
      rect.MaxX = x + parent_group->width - parent_group->margin.right - (parent_group->scroll.bar_vert ? UIOV_SCROLLBAR_WIDTH + parent_group->child_gap : 0);
      rect.MaxY = y + parent_group->height - parent_group->margin.bottom - (parent_group->scroll.bar_horiz ? UIOV_SCROLLBAR_HEIGHT + parent_group->child_gap : 0);
#else // UI_USE_SCROLLBARS
      rect.MaxX = x + parent_group->width - parent_group->margin.right;
      rect.MaxY = y + parent_group->height - parent_group->margin.bottom;
#endif // UI_USE_SCROLLBARS
#ifdef UI_USE_TABBED_GROUPS
    }
#endif // UI_USE_TABBED_GROUPS

    OrRectRegion(region, &rect);
  }

  return region;
}
///
///setClip(obj)
/******************************************************************************
 * Installs clipping for a given UIObject to the rectangle defined by it's x, *
 * y, width, height values regarding the clipping caused by it's parent       *
 * (containing) group object(s).                                              *
 ******************************************************************************/
VOID setClip(struct UIObject* obj)
{
  if (obj == ui_current_clipper) return;
  else {
    struct Region* region = NULL;
    struct Region* old_region;

    if (obj) {
      if (obj->type == UIOT_GROUP) {
        region = inheritClipRegion(obj);
      }
      else {
        struct Rectangle rect;
        WORD x = inheritX(obj);
        WORD y = inheritY(obj);

        region = inheritClipRegion(obj->parent);

        rect.MinX = x;
        rect.MinY = y;
        rect.MaxX = x + obj->width;
        rect.MaxY = y + obj->height;

        AndRectRegion(region, &rect);
      }
    }

    old_region = InstallClipRegion(ui_rastport->Layer, region);
    if (old_region) DisposeRegion(old_region);
    ui_current_clipper = obj;
  }
}
///
///removeClip()
/******************************************************************************
 * Removes the installed clipping by setClip()                                *
 ******************************************************************************/
VOID removeClip()
{
  struct Region* old_region = InstallClipRegion(ui_rastport->Layer, NULL);
  if (old_region) DisposeRegion(old_region);
  ui_current_clipper = NULL;
}
///

///initUI(screen_start, rastport, root, cycle_chain)
/******************************************************************************
 * Initializes the ui system to the given rastport and the ui tree (root)     *
 ******************************************************************************/
VOID initUI(ULONG screen_start, struct RastPort* rastport, struct UIO_Group* root, struct UIObject** cycle_chain)
{
  ui_screen_start = screen_start;
  ui_rastport = rastport;
  ui_rastport->DrawMode = JAM1;
  ui_current_clipper = NULL;
  ui_active_object = NULL;
  ui_hovered_object = NULL;
  ui_cycle_chain = cycle_chain;
  ui_cycle_chain_size = 0;
  ui_cycle_chain_index = 0;
  if (cycle_chain) {
    while (*cycle_chain++) ui_cycle_chain_size++;
    ui_cycle_chain_index = ui_cycle_chain_size;
  }
  NewList((struct List*)&ui_anim_list);
  if (!root->initialized) {
    setParents(root);
    setSizes(root);
    applyInheritance(root);
    root->initialized = TRUE;
  }
  root->draw((struct UIObject*)root);
}
///
///resetUI()
/******************************************************************************
 * De-Initializes the ui to NULL values.                                      *
 ******************************************************************************/
VOID resetUI()
{
  ui_rastport = NULL;
  ui_current_clipper = NULL;
  ui_active_object = NULL;
  if (ui_hovered_object) {
    ui_hovered_object->flags &= ~UIOF_HOVERED;
    ui_hovered_object = NULL;
  }
  if (ui_selected_object) {
    ui_selected_object->flags &= ~UIOF_SELECTED;
    ui_selected_object = NULL;
  }
  ui_cycle_chain = NULL;
  ui_cycle_chain_size = 0;
  ui_cycle_chain_index = 0;
  NewList((struct List*)&ui_anim_list);
}
///
///resetUILayout(root)
/******************************************************************************
 * Resets the position and sizes of all objects on the ui tree to zeros so    *
 * the layout can be done again using initUI().                               *
 * NOTE: Does not reset the position and sizes for the root object passed.    *
 * NOTE: Does not reset the sizes of groups with scrollers.                   *
 ******************************************************************************/
#ifdef UI_USE_RELAYOUTING
VOID resetUILayout(struct UIO_Group* root)
{
  struct UIObject** child = root->children;
  struct UIObject* obj;

  root->initialized = FALSE;

#ifdef UI_USE_TABBED_GROUPS
  if (root->flags & UIOF_TABBED) {
    root->u_tab_selector->x = 0;
    root->u_tab_selector->x = 0;
    root->u_tab_selector->width = 0;
    root->u_tab_selector->height = 0;
  }
  else {
#endif //UI_USE_TABBED_GROUPS
#ifdef UI_USE_SCROLLBARS
    if (root->scroll.bar_horiz) {
      root->scroll.bar_horiz->x = 0;
      root->scroll.bar_horiz->y = 0;
      root->scroll.bar_horiz->width = 0;
      root->scroll.bar_horiz->height = 0;
    }
    if (root->scroll.bar_vert) {
      root->scroll.bar_vert->x = 0;
      root->scroll.bar_vert->y = 0;
      root->scroll.bar_vert->width = 0;
      root->scroll.bar_vert->height = 0;
    }
#endif // UI_USE_SCROLLBARS
#ifdef UI_USE_TABBED_GROUPS
  }
#endif //UI_USE_TABBED_GROUPS

  while ((obj = *child++)) {
    if (obj->type == UIOT_GROUP) {
      obj->x = 0;
      obj->y = 0;
      if (!((struct UIO_Group*)obj)->scroll.bar_horiz) obj->width = 0;
      if (!((struct UIO_Group*)obj)->scroll.bar_vert) obj->height = 0;

      resetUILayout((struct UIO_Group*) obj);
    }
    else {
      obj->x = 0;
      obj->y = 0;
      obj->width = 0;
      obj->height = 0;
    }
  }
}
#endif // UI_USE_RELAYOUTING
///
///setParents(root)
/******************************************************************************
 * Sets the parent backpointers on every UIObject on the given ui tree (root) *
 ******************************************************************************/
VOID setParents(struct UIO_Group* root)
{
  struct UIObject** child = root->children;
  struct UIObject* obj;

#ifdef UI_USE_TABBED_GROUPS
  if (root->flags & UIOF_TABBED) {
    root->u_tab_selector->parent = (struct UIObject*)root;
  }
  else {
#endif //UI_USE_TABBED_GROUPS
#ifdef UI_USE_SCROLLBARS
    if (root->scroll.bar_horiz) root->scroll.bar_horiz->parent = (struct UIObject*)root;
    if (root->scroll.bar_vert) root->scroll.bar_vert->parent = (struct UIObject*)root;
#endif // UI_USE_SCROLLBARS
#ifdef UI_USE_TABBED_GROUPS
  }
#endif //UI_USE_TABBED_GROUPS

  while ((obj = *child++)) {
    obj->parent = (struct UIObject*)root;
    if (obj->type == UIOT_GROUP) {
      setParents((struct UIO_Group*) obj);
    }
  }
}
///
///setSizes(root)
/******************************************************************************
 * Calculates the sizes for UIObjects on the ui tree (root) which have the    *
 * calculate flags set calling their calcSize() functions (if any).           *
 ******************************************************************************/
VOID setSizes(struct UIO_Group* root)
{
  //If the root group does not have sizes set inherit them from the ui_rastport
  if (!root->parent) {
    if (root->flags & UIOF_INHERIT_X) root->x = ui_rastport->Layer->bounds.MinX;
    if (root->flags & UIOF_INHERIT_Y) root->y = ui_rastport->Layer->bounds.MinY;
    if (!root->width) root->width = ui_rastport->Layer->bounds.MaxX - root->x;
    if (!root->height) root->height = ui_rastport->Layer->bounds.MaxY - root->y;
  }

  if (root->calcSize) root->calcSize((struct UIObject*)root);
}
///
///applyInheritance(root)
/******************************************************************************
 * Calculates the offests and sizes for the UIObjects on the ui tree (root)   *
 * which have inherit flags (meaning they want their positions and/or sizes   *
 * set by their containing parent groups).                                    *
 * NOTE: There is probably duplicate code which algorithmically does the same *
 * calculations depending on group being horizontal/vertical for the sake of  *
 * readability This function can be cut in half if this can be resolved in a  *
 * readable way.                                                              *
 ******************************************************************************/
VOID applyInheritance(struct UIO_Group* root)
{
  struct UIObject** child = root->children;
  struct UIObject* obj;

#ifdef UI_USE_TABBED_GROUPS
  if (root->flags & UIOF_TABBED) {
    root->u_tab_selector->x = root->x;
    root->u_tab_selector->y = root->y;
    //width was inherited during sizeGroup()
    //height was calculated during sizeGroup()

    while ((obj = *child++)) {
      if (obj->flags & UIOF_INHERIT_X) obj->x = 0;
      if (obj->flags & UIOF_INHERIT_Y) obj->y = 0;
      if (obj->flags & UIOF_INHERIT_WIDTH) obj->width = root->width - (root->margin.left + root->margin.right);
      if (obj->flags & UIOF_INHERIT_HEIGHT) obj->height = root->height - (root->margin.top + root->margin.bottom);

      if (obj->type == UIOT_GROUP) {
        //Expand group's scroll sizes if required
        if (!(obj->flags & UIOF_TABBED)) {
          struct UIO_Group* group = (struct UIO_Group*)obj;
#ifdef UI_USE_SCROLLBARS
          UWORD extra_width = group->margin.left + group->margin.right + (group->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + group->child_gap) : 0);
          UWORD extra_height = group->margin.top + group->margin.bottom + (group->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + group->child_gap) : 0);
#else // UI_USE_SCROLLBARS
          UWORD extra_width = group->margin.left + group->margin.right;
          UWORD extra_height = group->margin.top + group->margin.bottom;
#endif // UI_USE_SCROLLBARS
          UWORD content_width = group->width - extra_width;
          UWORD content_height = group->height - extra_height;
          if (group->scroll.width < content_width) group->scroll.width = content_width;
          if (group->scroll.height < content_height) group->scroll.height = content_height;
        }

        applyInheritance((struct UIO_Group*) obj);
      }
    }
  }
  else {
#endif // UI_USE_TABBED_GROUPS
    if (root->flags & UIOF_HORIZONTAL) {
      ULONG num_children = 0;
      ULONG num_rows = root->columns;

      //Get num_children
      while ((obj = *child++)) num_children++;

      if (num_children) {
        //Get num_rows:
        UWORD num_columns = num_children / num_rows;
        UWORD num_columns_with_set_width = 0;
        UWORD num_rows_with_set_height = 0;
        UWORD total_set_row_height = 0;
        UWORD row_heights[16] = {0};
        UBYTE row_heights_set[16] = {0};
        UWORD total_content_height = 0;

        UWORD total_set_column_width = 0;
        UWORD total_gap_width = 0;

        UWORD legacy_width = 0;
        UWORD legacy_x = 0;
        UWORD legacy_y = 0;

        ULONG r;
        ULONG c;

        //Get all column widths
        child = root->children;
        for (r = 0; r < num_rows; r++) {
          ULONG row_height = 0;
          BOOL row_height_set = TRUE;

          child = &root->children[r];
          for (c = 0; c < num_columns; c++, child += num_rows) {
            obj = *child;
            if (obj->height > row_height) row_height = obj->height;
            if (obj->flags & UIOF_INHERIT_HEIGHT) row_height_set = FALSE;
          }
          row_heights[r] = row_height;
          if (row_height_set) {
            num_rows_with_set_height++;
            total_set_row_height += row_height;
          }
          row_heights_set[r] = row_height_set;
          total_content_height += row_height + root->child_gap;
        }
        total_content_height -= root->child_gap;

        //Equally distribute extra space if grup content height is larger
        if (root->scroll.height > total_content_height) {
          if (num_rows != num_rows_with_set_height) {
            ULONG extra_space_per_row = (root->scroll.height - total_content_height) / (num_rows - num_rows_with_set_height);
            for (r = 0; r < num_rows; r++) {
              if (!row_heights_set[r]) row_heights[r] += extra_space_per_row;
            }
          }
        }

        //Get all column widths
        child = root->children;
        for (c = 0; c < num_columns; c++) {
          ULONG column_width = 0;
          BOOL column_width_set = TRUE;

          for (r = 0; r < num_rows; r++, child++) {
            obj = *child;
            if (obj->width > column_width) column_width = obj->width;
            if (obj->flags & UIOF_INHERIT_WIDTH) column_width_set = FALSE;
          }
          if (column_width_set) {
            num_columns_with_set_width++;
            total_set_column_width += column_width;
          }
          total_gap_width += root->child_gap;
        }
        total_gap_width -= root->child_gap;

        if (num_columns != num_columns_with_set_width) {
          legacy_width = (root->scroll.width - total_gap_width - total_set_column_width) / (num_columns - num_columns_with_set_width);
        }

        //Do inheritance
        child = root->children;
        for (c = 0; c < num_columns; c++) {
          ULONG column_width = 0;
          legacy_y = 0;

          for (r = 0; r < num_rows; r++, child++) {
            obj = *child;
            if (obj->flags & UIOF_INHERIT_X) obj->x = legacy_x;
            if (obj->flags & UIOF_INHERIT_Y) obj->y = legacy_y;
            if (obj->flags & UIOF_INHERIT_WIDTH) obj->width = legacy_width;
            if (obj->flags & UIOF_INHERIT_HEIGHT) obj->height = row_heights[r];

            if (obj->type == UIOT_GROUP) {
              struct UIO_Group* group = (struct UIO_Group*)obj;

#ifdef UI_USE_TABBED_GROUPS
              if (obj->flags & UIOF_TABBED) {
                group->u_tab_selector->calcSize(group->u_tab_selector);
              }
              else {
#endif // UI_USE_TABBED_GROUPS
                //Expand child group's scroll sizes if required
#ifdef UI_USE_SCROLLBARS
                UWORD extra_width = group->margin.left + group->margin.right + (group->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + group->child_gap) : 0);
                UWORD extra_height = group->margin.top + group->margin.bottom + (group->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + group->child_gap) : 0);
#else // UI_USE_SCROLLBARS
                UWORD extra_width = group->margin.left + group->margin.right;
                UWORD extra_height = group->margin.top + group->margin.bottom;
#endif // UI_USE_SCROLLBARS
                UWORD content_width = group->width - extra_width;
                UWORD content_height = group->height - extra_height;
                if (group->scroll.width < content_width) group->scroll.width = content_width;
                if (group->scroll.height < content_height) group->scroll.height = content_height;
#ifdef UI_USE_TABBED_GROUPS
              }
#endif // UI_USE_TABBED_GROUPS

              applyInheritance((struct UIO_Group*) obj);
            }

            if (obj->width > column_width) column_width = obj->width;
            legacy_y += row_heights[r] + root->child_gap;
          }
          legacy_x += column_width + root->child_gap;
        }
        // A final touch to group height if final content layout does not fit!
        legacy_x -= root->child_gap;
        if (legacy_x >= root->scroll.width) {
          if (!(root->flags & UIOF_INHERIT_WIDTH)) root->width += (legacy_y - root->scroll.width);
          root->scroll.width = legacy_x;
        }
      }
    }
    else { // UIOF_VERTICAL
      ULONG num_children = 0;

      //Get num_children
      while ((obj = *child++)) num_children++;

      if (num_children) {
        //Get num_rows:
        UWORD num_rows = num_children / root->columns;
        UWORD num_rows_with_set_height = 0;
        UWORD num_columns_with_set_width = 0;
        UWORD total_set_column_width = 0;
        UWORD column_widths[16] = {0};
        UBYTE column_widths_set[16] = {0};
        UWORD total_content_width = 0;

        UWORD total_set_row_height = 0;
        UWORD total_gap_height = 0;

        UWORD legacy_height = 0;
        UWORD legacy_x = 0;
        UWORD legacy_y = 0;

        ULONG r;
        ULONG c;

        //Get all column widths
        child = root->children;
        for (c = 0; c < root->columns; c++) {
          ULONG column_width = 0;
          BOOL column_width_set = TRUE;

          child = &root->children[c];
          for (r = 0; r < num_rows; r++, child += root->columns) {
            obj = *child;
            if (obj->width > column_width) column_width = obj->width;
            if (obj->flags & UIOF_INHERIT_WIDTH) column_width_set = FALSE;
          }
          column_widths[c] = column_width;
          if (column_width_set) {
            num_columns_with_set_width++;
            total_set_column_width += column_width;
          }
          column_widths_set[c] = column_width_set;
          total_content_width += column_width + root->child_gap;
        }
        total_content_width -= root->child_gap;

        //Equally distribute extra space if grup content width is larger
        if (root->scroll.width > total_content_width) {
          if (root->columns != num_columns_with_set_width) {
            ULONG extra_space_per_column = (root->scroll.width - total_content_width) / (root->columns - num_columns_with_set_width);

            for (c = 0; c < root->columns; c++) {
              if (!column_widths_set[c]) column_widths[c] += extra_space_per_column;
            }
          }
        }

        //Get all row heights
        child = root->children;
        for (r = 0; r < num_rows; r++) {
          ULONG row_height = 0;
          BOOL row_height_set = TRUE;

          for (c = 0; c < root->columns; c++, child++) {
            obj = *child;
            if (obj->height > row_height) row_height = obj->height;
            if (obj->flags & UIOF_INHERIT_HEIGHT) row_height_set = FALSE;
          }
          if (row_height_set) {
            num_rows_with_set_height++;
            total_set_row_height += row_height;
          }
          total_gap_height += root->child_gap;
        }
        total_gap_height -= root->child_gap;

        if (num_rows != num_rows_with_set_height) {
          legacy_height = (root->scroll.height - total_gap_height - total_set_row_height) / (num_rows - num_rows_with_set_height);
        }

        //Do inheritance
        child = root->children;
        for (r = 0; r < num_rows; r++) {
          ULONG row_height = 0;
          legacy_x = 0;

          for (c = 0; c < root->columns; c++, child++) {
            obj = *child;
            if (obj->flags & UIOF_INHERIT_X) obj->x = legacy_x;
            if (obj->flags & UIOF_INHERIT_Y) obj->y = legacy_y;
            if (obj->flags & UIOF_INHERIT_WIDTH) obj->width = column_widths[c];
            if (obj->flags & UIOF_INHERIT_HEIGHT) obj->height = legacy_height;

            if (obj->type == UIOT_GROUP) {
              struct UIO_Group* group = (struct UIO_Group*)obj;

#ifdef UI_USE_TABBED_GROUPS
              if (obj->flags & UIOF_TABBED) {
                group->u_tab_selector->calcSize(group->u_tab_selector);
              }
              else {
#endif // UI_USE_TABBED_GROUPS
                //Expand child group's scroll sizes if required
#ifdef UI_USE_SCROLLBARS
                UWORD extra_width = group->margin.left + group->margin.right + (group->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + group->child_gap) : 0);
                UWORD extra_height = group->margin.top + group->margin.bottom + (group->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + group->child_gap) : 0);
#else // UI_USE_SCROLLBARS
                UWORD extra_width = group->margin.left + group->margin.right;
                UWORD extra_height = group->margin.top + group->margin.bottom;
#endif // UI_USE_SCROLLBARS
                UWORD content_width = group->width - extra_width;
                UWORD content_height = group->height - extra_height;
                if (group->scroll.width < content_width) group->scroll.width = content_width;
                if (group->scroll.height < content_height) group->scroll.height = content_height;
#ifdef UI_USE_TABBED_GROUPS
              }
#endif // UI_USE_TABBED_GROUPS

              applyInheritance((struct UIO_Group*) obj);
            }

            if (obj->height > row_height) row_height = obj->height;
            legacy_x += column_widths[c] + root->child_gap;
          }
          legacy_y += row_height + root->child_gap;
        }
        // A final touch to group height if final content layout does not fit!
        legacy_y -= root->child_gap;
        if (legacy_y >= root->scroll.height) {
          if (!(root->flags & UIOF_INHERIT_HEIGHT)) root->height += (legacy_y - root->scroll.height);
          root->scroll.height = legacy_y;
        }
      }
    }

#ifdef UI_USE_SCROLLBARS
    // Inherit position and sizes for group scrollbars
    {
      UWORD content_width = root->width - root->margin.left - root->margin.right - (root->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + root->child_gap) : 0);
      UWORD content_height = root->height - root->margin.top - root->margin.bottom - (root->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + root->child_gap) : 0);

      if (root->scroll.bar_vert) {
        root->scroll.bar_vert->x = content_width + root->child_gap;
        root->scroll.bar_vert->y = 0;
        root->scroll.bar_vert->width = UIOV_SCROLLBAR_WIDTH;
        root->scroll.bar_vert->height = content_height + (root->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + root->child_gap) : 0);
      }

      if (root->scroll.bar_horiz) {
        root->scroll.bar_horiz->x = 0;
        root->scroll.bar_horiz->y = content_height + root->child_gap;
        root->scroll.bar_horiz->width = content_width;
        root->scroll.bar_horiz->height = UIOV_SCROLLBAR_HEIGHT;
      }
    }
#endif // UI_USE_SCROLLBARS
#ifdef UI_USE_TABBED_GROUPS
  }
#endif // UI_USE_TABBED_GROUPS
}
///
///updateUI(root, pointer_x, pointer_y, pressed)
/******************************************************************************
 * Takes action on the objects on the UI tree regarding the pointer state.    *
 ******************************************************************************/
VOID updateUI(struct UIO_Group* root, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  static WORD old_pointer_x = 0;
  static WORD old_pointer_y = 0;
  static BOOL old_pressed = FALSE;
  struct UIObject* pointed = NULL;
  struct UIObject* animated = NULL;
  struct UIObject* next = NULL;
  BOOL disabled = FALSE;

  // Handle animations for animated ui objects
  for (animated = (struct UIObject*) ui_anim_list.mlh_Head; (next = (struct UIObject*) animated->node.mln_Succ); animated = next) {
    if (animated->anim.func) animated->anim.func(animated, pointer_x, pointer_y, pressed);
  }

  // Focus on the current active object if there is any
  if (ui_active_object && ui_active_object->onActive) {
    ui_active_object->onActive(ui_active_object, pointer_x, pointer_y, pressed);
    return;
  }

  if (old_pointer_x != pointer_x || old_pointer_y != pointer_y || old_pressed != pressed) {
    if (ui_selected_object && (ui_selected_object->flags & UIOF_SELECTED)) {
      ui_selected_object->flags &= ~UIOF_SELECTED;
      if (ui_selected_object->draw) ui_selected_object->draw(ui_selected_object);
    }

    pointed = findPointedObject(root, pointer_x, pointer_y, &disabled);

    if (!disabled) {
      if (pointed) {
        if (pointed != ui_hovered_object) {
          if (ui_hovered_object && ui_hovered_object->onHover) {
            ui_hovered_object->onHover(ui_hovered_object, pointer_x, pointer_y, FALSE);
          }
          ui_hovered_object = pointed;
          setIndexToObject(ui_hovered_object);
        }

        if (!disabled) {
          if (pointed->onHover) pointed->onHover(pointed, pointer_x, pointer_y, TRUE);
          if (pressed) {
            if (pointed->onActive) pointed->onActive(pointed, pointer_x, pointer_y, pressed);
            else if (pointed->onAcknowledge) pointed->onAcknowledge(pointed);
          }
        }
      }
      else {
        if (ui_hovered_object) {
          if (ui_hovered_object->onHover) ui_hovered_object->onHover(ui_hovered_object, pointer_x, pointer_y, FALSE);
          ui_hovered_object = NULL;
        }
      }
    }

    old_pointer_x = pointer_x;
    old_pointer_y = pointer_y;
    old_pressed = pressed;
  }
}
///
///findPointedObject(root, pointer_x, pointer_y)
/******************************************************************************
 * Finds and returns the ui object under pointer.                             *
 ******************************************************************************/
struct UIObject* findPointedObject(struct UIO_Group* root, WORD pointer_x, WORD pointer_y, BOOL* disabled)
{
  struct UIObject** child = root->children;
  struct UIObject* obj;
  struct UIObject* pointed = NULL;

  WORD x = inheritX((struct UIObject*)root);
  WORD y = inheritY((struct UIObject*)root);
  WORD x2 = x + root->width;
  WORD y2 = y + root->height;

  if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
    pointed = (struct UIObject*)root;

    if (pointed->flags & UIOF_DISABLED) {
      *disabled = TRUE;
    }
    else {
#ifdef UI_USE_TABBED_GROUPS
      if (root->flags & UIOF_TABBED) {
        if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y + root->u_tab_selector->height)) {
          return root->u_tab_selector;
        }
      }
      else {
#endif // UI_USE_TABBED_GROUPS
#ifdef UI_USE_SCROLLBARS
        if (root->scroll.bar_horiz) {
          WORD x = inheritX(root->scroll.bar_horiz) + root->scroll.x;
          WORD y = inheritY(root->scroll.bar_horiz) + root->scroll.y;
          WORD x2 = x + root->scroll.bar_horiz->width;
          WORD y2 = y + root->scroll.bar_horiz->height;

          if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
            return root->scroll.bar_horiz;
          }
        }
        if (root->scroll.bar_vert) {
          WORD x = inheritX(root->scroll.bar_vert) + root->scroll.x;
          WORD y = inheritY(root->scroll.bar_vert) + root->scroll.y;
          WORD x2 = x + root->scroll.bar_vert->width;
          WORD y2 = y + root->scroll.bar_vert->height;

          if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
            return root->scroll.bar_vert;
          }
        }

        if (root->scroll.bar_horiz) y2 -= (root->child_gap + UIOV_SCROLLBAR_HEIGHT);
        if (root->scroll.bar_vert) x2 -= (root->child_gap + UIOV_SCROLLBAR_WIDTH);
#endif // UI_USE_SCROLLBARS
#ifdef UI_USE_TABBED_GROUPS
      }
#endif // UI_USE_TABBED_GROUPS
    }

    x += root->margin.left;
    y += root->margin.top;
    x2 -= root->margin.right;
    y2 -= root->margin.bottom;

    if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
#ifdef UI_USE_TABBED_GROUPS
      if (root->flags & UIOF_TABBED) {
        obj = child[root->u_tab_selected];
        if (obj->type == UIOT_GROUP) {
          obj = findPointedObject((struct UIO_Group*) obj, pointer_x, pointer_y, disabled);
          if (obj) pointed = obj;
        }
        else {
          x = inheritX(obj);
          y = inheritY(obj);
          x2 = x + obj->width;
          y2 = y + obj->height;

          if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
            pointed = obj;
            if (*disabled == FALSE) *disabled = pointed->flags & UIOF_DISABLED;
          }
        }
      }
      else {
#endif // UI_USE_TABBED_GROUPS
        while ((obj = *child++)) {
          if (obj->type == UIOT_GROUP) {
            obj = findPointedObject((struct UIO_Group*) obj, pointer_x, pointer_y, disabled);
            if (obj) {
              pointed = obj;
              if (*disabled == FALSE) *disabled = pointed->flags & UIOF_DISABLED;
              break;
            }
          }
          else {
            x = inheritX(obj);
            y = inheritY(obj);
            x2 = x + obj->width;
            y2 = y + obj->height;

            if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
              pointed = obj;
              if (*disabled == FALSE) *disabled = pointed->flags & UIOF_DISABLED;
              break;
            }
          }
        }
#ifdef UI_USE_TABBED_GROUPS
      }
#endif // UI_USE_TABBED_GROUPS
    }
  }

  return pointed;
}
///

///selectObjectAtIndex(index)
/******************************************************************************
 * Makes the object at the index on cycle chain the selected object.          *
 ******************************************************************************/
struct UIObject* selectObjectAtIndex(ULONG index)
{
  if (ui_cycle_chain) {
    if (index > ui_cycle_chain_size) index = ui_cycle_chain_size;
    ui_cycle_chain_index = index;
    if (ui_selected_object) {
      ui_selected_object->flags &= ~UIOF_SELECTED;
      if (ui_selected_object->draw) ui_selected_object->draw(ui_selected_object);
    }
    ui_selected_object = ui_cycle_chain[ui_cycle_chain_index];
    if (ui_selected_object) {
      ui_selected_object->flags |= UIOF_SELECTED;
      makeObjectVisible(ui_selected_object);
    }
  }

  return ui_selected_object;
}
///
///selectObject(object)
/******************************************************************************
 * Makes the given object the selected ui object. Sets the cycle chain index  *
 * to this object, if the object is on the cycle chain.                       *
 ******************************************************************************/
ULONG selectObject(struct UIObject* object)
{
  if (ui_selected_object != object) {
    if (ui_selected_object && (ui_selected_object->flags & UIOF_SELECTED)) {
      ui_selected_object->flags &= ~UIOF_SELECTED;
      if (ui_selected_object->draw) ui_selected_object->draw(ui_selected_object);
    }
    ui_selected_object = object;
    if (ui_selected_object) {
      ui_selected_object->flags |= UIOF_SELECTED;
      makeObjectVisible(ui_selected_object);
    }

    return setIndexToObject(ui_selected_object);
  }

  return ui_cycle_chain_size;
}
///
///setIndexToObject(object)
/******************************************************************************
 * Sets the cycle chain index to the object given if the object is in the     *
 * cycle chain.                                                               *
 ******************************************************************************/
ULONG setIndexToObject(struct UIObject* object)
{
  if (ui_cycle_chain) {
    ULONG i;
    for (i = 0; i < ui_cycle_chain_size; i++) {
      if (object == ui_cycle_chain[i]) {
        ui_cycle_chain_index = i;
        return i;
      }
    }
  }

  return ui_cycle_chain_size;
}
///
///getSelectedIndex()
/******************************************************************************
 * Returns the index of the currently selected ui_object on the cycle chain.  *
 ******************************************************************************/
ULONG getSelectedIndex()
{
  return ui_cycle_chain_index;
}
///
///getSelectedObject()
/******************************************************************************
 * Returns the currently selected ui object (if it is actually selected).     *
 ******************************************************************************/
struct UIObject* getSelectedObject()
{
  if (ui_selected_object && ui_selected_object->flags & UIOF_SELECTED)
    return ui_selected_object;
  else
    return NULL;
}
///
///nextObject()
/******************************************************************************
 * Selects the next object on the cycle chain.                                *
 ******************************************************************************/
struct UIObject* nextObject()
{
  if (ui_cycle_chain) {
    ui_cycle_chain_index++;
    if (ui_cycle_chain_index > ui_cycle_chain_size) ui_cycle_chain_index = 0;

    if (ui_selected_object) {
      ui_selected_object->flags &= ~UIOF_SELECTED;
      if (ui_selected_object->draw) ui_selected_object->draw(ui_selected_object);
    }
    ui_selected_object = ui_cycle_chain[ui_cycle_chain_index];
    if (ui_selected_object) {
      ui_selected_object->flags |= UIOF_SELECTED;
      makeObjectVisible(ui_selected_object);
    }
  }

  return ui_selected_object;
}
///
///prevObject()
/******************************************************************************
 * Selects the previous object on the cycle chain.                            *
 ******************************************************************************/
struct UIObject* prevObject()
{
  if (ui_cycle_chain) {
    if (ui_cycle_chain_index == 0) ui_cycle_chain_index = ui_cycle_chain_size;
    else ui_cycle_chain_index--;

    if (ui_selected_object) {
      ui_selected_object->flags &= ~UIOF_SELECTED;
      if (ui_selected_object->draw) ui_selected_object->draw(ui_selected_object);
    }
    ui_selected_object = ui_cycle_chain[ui_cycle_chain_index];
    if (ui_selected_object) {
      ui_selected_object->flags |= UIOF_SELECTED;
      makeObjectVisible(ui_selected_object);
    }
  }

  return ui_selected_object;
}
///
///activateObject()
/******************************************************************************
 * Activates the currently selected object on the cycle chain.                *
 ******************************************************************************/
struct UIObject* activateObject()
{
  if (ui_selected_object && ui_selected_object->onActive) {
    WORD x1 = inheritX(ui_selected_object);
    WORD y1 = inheritY(ui_selected_object);
    WORD x2 = x1 + ui_selected_object->width;
    WORD y2 = y1 + ui_selected_object->height;

    switch (ui_selected_object->type) {
      case UIOT_BUTTON:
      case UIOT_CYCLEBOX:
        ui_selected_object->onActive(ui_selected_object, x1, y1, FALSE);
      break;
      case UIOT_CHECKBOX:
        ui_selected_object->onActive(ui_selected_object, x1, y1, TRUE);
      break;
      case UIOT_STRING:
      case UIOT_INTEGER:
        ui_selected_object->onActive(ui_selected_object, x2 - 1, y2 - 1, TRUE);
      break;
    }
  }

  return ui_selected_object;
}
///

///sizeGroup(self)
/******************************************************************************
 * Default size method for group objects.                                     *
 * NOTE: There is probably duplicate code which algorithmically does the same *
 * calculations depending on group being horizontal/vertical for the sake of  *
 * readability This function can be cut in half if this can be resolved in a  *
 * readable way.                                                              *
 ******************************************************************************/
VOID sizeGroup(struct UIObject* self)
{
  struct UIO_Group* group = (struct UIO_Group*) self;
  struct UIObject** child = group->children;
  struct UIObject* obj;

#ifdef UI_USE_TABBED_GROUPS
  if (group->flags & UIOF_TABBED) { // TABBED GROUP
    WORD extra_width;
    WORD content_width = 0;
    WORD extra_height;
    WORD content_height = 0;

    //Expand margins for tab frames
    if (group->flags & UIOF_FRAMED) {
      group->margin.left += UIOV_TAB_FRAME_WIDTH + UIOV_GROUP_FRAME_MARGIN;
      group->margin.right += UIOV_TAB_FRAME_WIDTH + UIOV_GROUP_FRAME_MARGIN;
    }
    extra_width = group->margin.left + group->margin.right;

    //Expand top margin for stuff displayed in the margin areas
    group->u_tab_selector->calcSize(group->u_tab_selector);
    group->margin.top += group->u_tab_selector->height + UIOV_GROUP_FRAME_MARGIN;
    if (group->flags & UIOF_FRAMED) {
      group->margin.bottom += UIOV_TAB_FRAME_HEIGHT + UIOV_GROUP_FRAME_MARGIN;
    }
    extra_height = group->margin.top + group->margin.bottom;

    while ((obj = *child++)) {
      if (obj->calcSize) obj->calcSize(obj);
      content_height = MAX(content_height, obj->height);
    }

    child = group->children;
    while ((obj = *child++)) {
      content_width = MAX(content_width, obj->width);
    }

    if (group->width) {
      WORD set_content_width = (WORD)group->width - extra_width;
      if (set_content_width < 0) {
        set_content_width = UIOV_GROUP_MIN_WIDTH;
        group->width = set_content_width + extra_width;
      }
    }
    else {
      group->width = content_width + extra_width;
    }

    if (group->height) {
      WORD set_content_height = (WORD)group->height - extra_height;
      if (set_content_height < 0) {
        set_content_height = UIOV_GROUP_MIN_HEIGHT;
        group->height = set_content_height + extra_height;
      }
    }
    else {
      group->height = content_height + extra_height;
    }
  }
  else { // REGULAR LAYOUT GROUP
#endif // UI_USE_TABBED_GROUPS
    WORD content_width = 0;
    WORD extra_width;
    WORD content_height = 0;
    WORD extra_height;
    UWORD num_children = 0;

    //Expand margins for stuff displayed in the margin areas
    if (group->flags & UIOF_FRAMED) {
      group->margin.left += UIOV_GROUP_FRAME_WIDTH + UIOV_GROUP_FRAME_MARGIN;
      group->margin.right += UIOV_GROUP_FRAME_WIDTH + UIOV_GROUP_FRAME_MARGIN;
    }
#ifdef UI_USE_SCROLLBARS
    extra_width = group->margin.left + group->margin.right + (group->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + group->child_gap) : 0);
#else // UI_USE_SCROLLBARS
    extra_width = group->margin.left + group->margin.right;
#endif // UI_USE_SCROLLBARS

    if (group->flags & UIOF_FRAMED) {
      if (group->flags & UIOF_TITLED) group->margin.top += ui_rastport->Font->tf_YSize;
      else group->margin.top += UIOV_GROUP_FRAME_HEIGHT + UIOV_GROUP_FRAME_MARGIN;
      group->margin.bottom += UIOV_GROUP_FRAME_HEIGHT + UIOV_GROUP_FRAME_MARGIN;
    }
#ifdef UI_USE_SCROLLBARS
    extra_height = group->margin.top + group->margin.bottom + (group->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + group->child_gap) : 0);
#else // UI_USE_SCROLLBARS
    extra_height = group->margin.top + group->margin.bottom;
#endif // UI_USE_SCROLLBARS

    //Get num_children
    while ((obj = *child++)) num_children++;
    if (num_children) {
      if (!group->columns) group->columns = 1;

      if (group->flags & UIOF_HORIZONTAL) {
        UWORD num_rows = group->columns; // union value!
        UWORD num_columns = num_children / num_rows;

        if (num_children % num_rows) {
          //If there is an un-filled column at the end remove those extra children
          num_children = num_columns * num_rows;
          child = &group->children[num_children];
          while (*child) {*child = NULL; child++;}
        }
        if (num_children) {
          ULONG r;
          ULONG c;

          for (r = 0; r < num_rows; r++) {
            UWORD row_height = 0;

            child = &group->children[r];
            for (c = 0; c < num_columns; c++, child += num_rows) {
              obj = *child;
              if (obj->calcSize) obj->calcSize(obj);
              if (obj->height > row_height) row_height = obj->height;
            }
            content_height += row_height + group->child_gap;
          }
          content_height -= group->child_gap;


          child = group->children;
          for (c = 0; c < num_columns; c++) {
            UWORD column_width = 0;

            for (r = 0; r < num_rows; r++, child++) {
              obj = *child;
              if (obj->width > column_width) column_width = obj->width;
            }
            content_width += column_width + group->child_gap;
          }
          content_width -= group->child_gap;
        }
      }
      else { // UIOF_VERTICAL
        UWORD num_rows = num_children / group->columns;
        if (num_children % group->columns) {
          //If there is an un-filled row at the end remove those extra children
          num_children = num_rows * group->columns;
          child = &group->children[num_children];
          while (*child) {*child = NULL; child++;}
        }

        if (num_children) {
          ULONG r;
          ULONG c;

          for (c = 0; c < group->columns; c++) {
            UWORD column_width = 0;

            child = &group->children[c];
            for (r = 0; r < num_rows; r++, child += group->columns) {
              obj = *child;
              if (obj->calcSize) obj->calcSize(obj);
              if (obj->width > column_width) column_width = obj->width;
            }
            content_width += column_width + group->child_gap;
          }
          content_width -= group->child_gap;

          child = group->children;
          for (r = 0; r < num_rows; r++) {
            UWORD row_height = 0;

            for (c = 0; c < group->columns; c++, child++) {
              obj = *child;
              if (obj->height > row_height) row_height = obj->height;
            }
            content_height += row_height + group->child_gap;
          }
          content_height -= group->child_gap;
        }
      }
    }

    // An already set group width overrides calculation and sets scroll values instead
    if (group->width) {
      WORD set_content_width = (WORD)group->width - extra_width;
      if (set_content_width < 0) {
        set_content_width = UIOV_GROUP_MIN_WIDTH;
        group->width = set_content_width + extra_width;
      }
      if (set_content_width < content_width) {
        if (group->scroll.width < content_width) {
          group->scroll.width = content_width;
        }
      }
      else {
        if (group->scroll.width < set_content_width) {
          group->scroll.width = set_content_width;
        }
      }
    }
    else {
      group->width = content_width + extra_width;
      if (group->scroll.width < content_width) {
        group->scroll.width = content_width;
      }
    }

    // An already set group height overrides calculation and sets scroll values instead
    if (group->height) {
      WORD set_content_height = (WORD)group->height - extra_height;
      if (set_content_height < 0) {
        set_content_height = UIOV_GROUP_MIN_HEIGHT;
        group->height = set_content_height + extra_height;
      }
      if (set_content_height < content_height) {
        if (group->scroll.height < content_height) {
          group->scroll.height = content_height;
        }
      }
      else {
        if (group->scroll.height < set_content_height) {
          group->scroll.height = set_content_height;
        }
      }
    }
    else {
      group->height = content_height + extra_height;
      if (group->scroll.height < content_height) {
        group->scroll.height = content_height;
      }
    }
#ifdef UI_USE_TABBED_GROUPS
  }
#endif // UI_USE_TABBED_GROUPS
}
///
///scrollGroupVert(self, scroll_value)
/******************************************************************************
 * Scolls the contents of a group object vertically as much pixels as given   *
 * in the scroll_value argument. Negative values scroll up.                   *
 ******************************************************************************/
#ifdef UI_USE_SCROLLING
VOID scrollGroupVert(struct UIO_Group* self, WORD scroll_value)
{
  // Scroll Up
  if (scroll_value < 0) {
    WORD new_scroll_y = self->scroll.y + scroll_value;

    if (new_scroll_y < 0) {
      new_scroll_y = 0;
      scroll_value = -self->scroll.y;
    }

    if (!scroll_value) return;
    self->scroll.y += scroll_value;

#ifdef UI_SCROLL_USING_REDRAW
    if (self->draw) self->draw((struct UIObject*)self);
#else // SCROLL USING BLITTER
    {
      struct UIObject** child = self->children;
      struct UIObject* obj;
      WORD x1 = inheritX((struct UIObject*)self) + self->margin.left;
      WORD y1 = inheritY((struct UIObject*)self) + self->margin.top;
#ifdef UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right + (self->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + self->child_gap) : 0));
      UWORD height = self->height - (self->margin.top + self->margin.bottom + (self->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + self->child_gap) : 0));
#else // UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right);
      UWORD height = self->height - (self->margin.top + self->margin.bottom);
#endif // UI_USE_SCROLLBARS
      if (-scroll_value >= height) {
        if (self->draw) self->draw((struct UIObject*)self);
      }
      else {
        UWORD scroll_height = height + scroll_value;
        struct Region* clip_region, *old_region;
        struct Rectangle rect;

        setClip(self->parent);

#ifdef UI_USE_SCROLLBARS
        if (self->scroll.bar_vert) {
          self->scroll.bar_vert->draw(self->scroll.bar_vert);
        }
#endif // UI_USE_SCROLLBARS

        //Copy scrollable area up
        ClipBlit(ui_rastport, x1, y1, ui_rastport, x1, y1 - scroll_value, width + 1, scroll_height + 1, (ABC|ABNC));
        //Define vacated area
        rect.MinX = x1;
        rect.MinY = y1;
        rect.MaxX = x1 + width;
        rect.MaxY = y1 - scroll_value;
        //Clean up vacated area
        SetAPen(ui_rastport, UIPEN_BACKGROUND);
        RectFill(ui_rastport, x1, y1, rect.MaxX, rect.MaxY);

        //Set clipping to vacated area
        clip_region = inheritClipRegion(self->parent);
        AndRectRegion(clip_region, &rect);
        old_region = InstallClipRegion(ui_rastport->Layer, clip_region);
        if (old_region) DisposeRegion(old_region);
        ui_current_clipper = (struct UIObject*)self;

        //Draw child objects that fall into vacated area
        while ((obj = *child++)) {
          if (obj->y + obj->height < self->scroll.y) continue;
          if (obj->y > self->scroll.y - scroll_value) continue;
          if (obj->draw) obj->draw(obj);
        }
        removeClip();
      }
    }
#endif // SCROLL USING BLITTER
  }

  // Scroll Down
  if (scroll_value > 0) {
#ifdef UI_USE_SCROLLBARS
    WORD content_height = self->height - self->margin.top - self->margin.bottom - (self->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + self->child_gap) : 0);
#else // UI_USE_SCROLLBARS
    WORD content_height = self->height - self->margin.top - self->margin.bottom;
#endif // UI_USE_SCROLLBARS
    WORD new_scroll_y = self->scroll.y + scroll_value;
    WORD scroll_y_max = self->scroll.height - content_height;

    if (new_scroll_y > scroll_y_max) {
      new_scroll_y = scroll_y_max;
      scroll_value = scroll_y_max - self->scroll.y;
    }

    if (!scroll_value) return;
    self->scroll.y += scroll_value;

#ifdef UI_SCROLL_USING_REDRAW
    if (self->draw) self->draw((struct UIObject*)self);
#else // SCROLL USING BLITTER
    {
      struct UIObject** child = self->children;
      struct UIObject* obj;
      WORD x1 = inheritX((struct UIObject*)self) + self->margin.left;
      WORD y1 = inheritY((struct UIObject*)self) + self->margin.top;
#ifdef UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right + (self->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + self->child_gap) : 0));
      UWORD height = self->height - (self->margin.top + self->margin.bottom + (self->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + self->child_gap) : 0));
#else // UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right);
      UWORD height = self->height - (self->margin.top + self->margin.bottom);
#endif // UI_USE_SCROLLBARS
      if (scroll_value >= height) {
        if (self->draw) self->draw((struct UIObject*)self);
      }
      else {
        UWORD scroll_height = height - scroll_value;
        struct Region* clip_region, *old_region;
        struct Rectangle rect;

        setClip(self->parent);

#ifdef UI_USE_SCROLLBARS
        if (self->scroll.bar_vert) {
          self->scroll.bar_vert->draw(self->scroll.bar_vert);
        }
#endif // UI_USE_SCROLLBARS

        ClipBlit(ui_rastport, x1, y1 + scroll_value, ui_rastport, x1, y1, width + 1, scroll_height + 1, (ABC|ABNC));
        rect.MinX = x1;
        rect.MinY = y1 + height - scroll_value;
        rect.MaxX = x1 + width;
        rect.MaxY = y1 + height;
        SetAPen(ui_rastport, UIPEN_BACKGROUND);
        RectFill(ui_rastport, x1, rect.MinY,  rect.MaxX, rect.MaxY);

        clip_region = inheritClipRegion(self->parent);
        AndRectRegion(clip_region, &rect);
        old_region = InstallClipRegion(ui_rastport->Layer, clip_region);
        if (old_region) DisposeRegion(old_region);
        ui_current_clipper = (struct UIObject*)self;

        while ((obj = *child++)) {
          if (obj->y + obj->height < self->scroll.y + scroll_height) continue;
          if (obj->y > self->scroll.y + height) continue;
          if (obj->draw) obj->draw(obj);
        }
        removeClip();
      }
    }
#endif // SCROLL USING BLITTER
  }
}
#endif // UI_USE_SCROLLING
///
///scrollGroupHoriz(self, scroll_value)
/******************************************************************************
 * Scolls the contents of a group object horizontally as much pixels as given *
 * in the scroll_value argument. Negative values scoll left.                  *
 ******************************************************************************/
#ifdef UI_USE_SCROLLING
VOID scrollGroupHoriz(struct UIO_Group* self, WORD scroll_value)
{
  // Scroll left
  if (scroll_value < 0) {
    WORD new_scroll_x = self->scroll.x + scroll_value;

    if (new_scroll_x < 0) {
      new_scroll_x = 0;
      scroll_value = -self->scroll.x;
    }

    if (!scroll_value) return;
    self->scroll.x += scroll_value;

    #ifdef UI_SCROLL_USING_REDRAW
    if (self->draw) self->draw((struct UIObject*)self);
    #else // SCROLL USING BLITTER
    {
      struct UIObject** child = self->children;
      struct UIObject* obj;
      WORD x1 = inheritX((struct UIObject*)self) + self->margin.left;
      WORD y1 = inheritY((struct UIObject*)self) + self->margin.top;
#ifdef UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right + (self->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + self->child_gap) : 0));
      UWORD height = self->height - (self->margin.top + self->margin.bottom + (self->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + self->child_gap) : 0));
#else // UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right);
      UWORD height = self->height - (self->margin.top + self->margin.bottom);
#endif // UI_USE_SCROLLBARS
      if (-scroll_value >= width) {
        if (self->draw) self->draw((struct UIObject*)self);
      }
      else {
        UWORD scroll_width = width + scroll_value;
        struct Region* clip_region, *old_region;
        struct Rectangle rect;

        setClip(self->parent);

#ifdef UI_USE_SCROLLBARS
        if (self->scroll.bar_horiz) {
          self->scroll.bar_horiz->draw(self->scroll.bar_horiz);
        }
#endif // UI_USE_SCROLLBARS

        //Copy blitable area left
        ClipBlit(ui_rastport, x1, y1, ui_rastport, x1 - scroll_value, y1, scroll_width + 1, height + 1, (ABC|ABNC));
        //Define vacated area
        rect.MinX = x1;
        rect.MinY = y1;
        rect.MaxX = x1 - scroll_value;
        rect.MaxY = y1 + height;
        //Clean up vacated area
        SetAPen(ui_rastport, UIPEN_BACKGROUND);
        RectFill(ui_rastport, x1, y1, rect.MaxX, rect.MaxY);

        //Set clipping to vacated area
        clip_region = inheritClipRegion(self->parent);
        AndRectRegion(clip_region, &rect);
        old_region = InstallClipRegion(ui_rastport->Layer, clip_region);
        if (old_region) DisposeRegion(old_region);
        ui_current_clipper = (struct UIObject*)self;

        //Draw child objects that fall into vacated area
        while ((obj = *child++)) {
          if (obj->x + obj->width < self->scroll.x) continue;
          if (obj->x > self->scroll.x - scroll_value) continue;
          if (obj->draw) obj->draw(obj);
        }
        removeClip();
      }
    }
#endif // SCROLL USING BLITTER
  }

  // Scroll right
  if (scroll_value > 0) {
#ifdef UI_USE_SCROLLBARS
    WORD content_width = self->width - self->margin.left - self->margin.right - (self->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + self->child_gap) : 0);
#else // UI_USE_SCROLLBARS
    WORD content_width = self->width - self->margin.left - self->margin.right;
#endif // UI_USE_SCROLLBARS
    WORD new_scroll_x = self->scroll.x + scroll_value;
    WORD scroll_x_max = self->scroll.width - content_width;

    if (new_scroll_x > scroll_x_max) {
      new_scroll_x = scroll_x_max;
      scroll_value = scroll_x_max - self->scroll.x;
    }

    if (!scroll_value) return;
    self->scroll.x += scroll_value;

#ifdef UI_SCROLL_USING_REDRAW
    if (self->draw) self->draw((struct UIObject*)self);
#else // SCROLL USING BLITTER
    {
      struct UIObject** child = self->children;
      struct UIObject* obj;
      WORD x1 = inheritX((struct UIObject*)self) + self->margin.left;
      WORD y1 = inheritY((struct UIObject*)self) + self->margin.top;
#ifdef UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right + (self->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + self->child_gap) : 0));
      UWORD height = self->height - (self->margin.top + self->margin.bottom + (self->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + self->child_gap) : 0));
#else // UI_USE_SCROLLBARS
      UWORD width = self->width - (self->margin.left + self->margin.right);
      UWORD height = self->height - (self->margin.top + self->margin.bottom);
#endif // UI_USE_SCROLLBARS
      if (scroll_value >= width) {
        if (self->draw) self->draw((struct UIObject*)self);
      }
      else {
        UWORD scroll_width = width - scroll_value;
        struct Region* clip_region, *old_region;
        struct Rectangle rect;

        setClip(self->parent);

#ifdef UI_USE_SCROLLBARS
        if (self->scroll.bar_horiz) {
          self->scroll.bar_horiz->draw(self->scroll.bar_horiz);
        }
#endif // UI_USE_SCROLLBARS

        ClipBlit(ui_rastport, x1 + scroll_value, y1, ui_rastport, x1, y1, scroll_width + 1, height + 1, (ABC|ABNC));
        rect.MinX = x1 + width - scroll_value;
        rect.MinY = y1;
        rect.MaxX = x1 + width;
        rect.MaxY = y1 + height;
        SetAPen(ui_rastport, UIPEN_BACKGROUND);
        RectFill(ui_rastport, rect.MinX, y1, rect.MaxX, rect.MaxY);

        clip_region = inheritClipRegion(self->parent);
        AndRectRegion(clip_region, &rect);
        old_region = InstallClipRegion(ui_rastport->Layer, clip_region);
        if (old_region) DisposeRegion(old_region);
        ui_current_clipper = (struct UIObject*)self;

        while ((obj = *child++)) {
          if (obj->x + obj->width < self->scroll.x + scroll_width) continue;
          if (obj->x > self->scroll.x + width) continue;
          if (obj->draw) obj->draw(obj);
        }
        removeClip();
      }
    }
    #endif
  }
}
#endif // UI_USE_SCROLLING
///
///makeObjectVisible(object)
/******************************************************************************
 * Makes the object visible by changing the scroll values (or selected tab)   *
 * of its parent groups going back until the root group.                      *
 * NOTE: Redraws the object (and its parent groups if required).              *
 ******************************************************************************/
VOID makeObjectVisible(struct UIObject* object)
{
  struct UIO_Group* parent = (struct UIO_Group*)object->parent;
  struct UIObject* child = object;
  struct UIObject* object_to_redraw = object;
  WORD x1 = object->x;
  WORD y1 = object->y;
  WORD x2 = x1 + object->width;
  WORD y2 = y1 + object->height;
#ifdef UI_USE_TABBED_GROUPS
  BOOL tab_change = FALSE;
#endif // UI_USE_TABBED_GROUPS

  while (parent) {
#ifdef UI_USE_TABBED_GROUPS
    if (parent->flags & UIOF_TABBED) {
      struct UIObject** children = parent->children;
      struct UIObject* tab_child;
      UWORD tab_num = 0;

      while ((tab_child = *children++)) {
        if (tab_child == child) break;
        tab_num++;
      }

      if (parent->u_tab_selected != tab_num) {
        object_to_redraw = (struct UIObject*)parent;
        parent->u_tab_selected = tab_num;
        tab_change = TRUE;
      }

      x1 += parent->x + parent->margin.left;
      y1 += parent->y + parent->margin.top;
      x2 = x1 + object->width;
      y2 = y1 + object->height;
    }
    else {
#endif // UI_USE_TABBED_GROUPS
      //visible rectangle of the parent group
      WORD pg_x1 = parent->scroll.x;
      WORD pg_y1 = parent->scroll.y;
#ifdef UI_USE_SCROLLBARS
      WORD pg_x2 = pg_x1 + parent->width - parent->margin.left - parent->margin.right - (parent->scroll.bar_vert ? UIOV_SCROLLBAR_WIDTH + parent->child_gap : 0);
      WORD pg_y2 = pg_y1 + parent->height - parent->margin.top - parent->margin.bottom - (parent->scroll.bar_horiz ? UIOV_SCROLLBAR_HEIGHT + parent->child_gap : 0);
#else // UI_USE_SCROLLBARS
      WORD pg_x2 = pg_x1 + parent->width - parent->margin.left - parent->margin.right;
      WORD pg_y2 = pg_y1 + parent->height - parent->margin.top - parent->margin.bottom;
#endif // UI_USE_SCROLLBARS

      //make right edge of the object visible
      if (x2 > pg_x2) {
        object_to_redraw = (struct UIObject*)parent;
        parent->scroll.x += x2 - pg_x2;
      }
      //make bottom edge of the object visible
      if (y2 > pg_y2) {
        object_to_redraw = (struct UIObject*)parent;
        parent->scroll.y += y2 - pg_y2;
      }
      //make left edge of the object visible
      if (x1 < pg_x1) {
        object_to_redraw = (struct UIObject*)parent;
        parent->scroll.x -= pg_x1 - x1;
      }
      //make top edge of the object visible
      if (y1 < pg_y1) {
        object_to_redraw = (struct UIObject*)parent;
        parent->scroll.y -= pg_y1 - y1;
      }

      x1 += parent->x + parent->margin.left - parent->scroll.x;
      y1 += parent->y + parent->margin.top - parent->scroll.y;
      x2 = x1 + object->width;
      y2 = y1 + object->height;
#ifdef UI_USE_TABBED_GROUPS
    }
#endif // UI_USE_TABBED_GROUPS

    child = (struct UIObject*)parent;
    parent = (struct UIO_Group*)parent->parent;
  }

#ifdef UI_USE_TABBED_GROUPS
  //if making this object visible caused a tab change deactivate and un-hover objects
  if (tab_change) {
    if (ui_active_object) {
      if (ui_active_object->onActive) ui_active_object->onActive(ui_active_object, 0, 0, FALSE);
      ui_active_object = NULL;
    }
    if (ui_hovered_object) {
      if (ui_hovered_object->onHover) ui_hovered_object->onHover(ui_hovered_object, 0, 0, FALSE);
      ui_hovered_object = NULL;
    }
  }
#endif // UI_USE_TABBED_GROUPS
  //redraw the object (or its parents if needed)
  if (object_to_redraw->draw) object_to_redraw->draw(object_to_redraw);
}
///

/*************************
 * SIZE CALCULATORS      *
 *************************/
///sizeTabSelector(self)
/******************************************************************************
 * UTILITY FUNCTION: Distributes tab widths so that more is given to those    *
 * which need more.                                                           *
 ******************************************************************************/
#ifdef UI_USE_TABBED_GROUPS
VOID distribute_tab_widths(struct UIObject** children, UWORD count, WORD total_width, UBYTE* result) {
  UBYTE* original = AllocMem(count, MEMF_ANY);

  if (original) {
    WORD total_original = 0;
    WORD fixed_total = 0;
    WORD shrinkable_total = 0;
    WORD shrinkable_count = 0;
    WORD average = 0;
    WORD remaining;
    WORD per_shrink;
    WORD remainder;
    LONG i;

    // Get all original label widths into an array
    for (i = 0; i < count; ++i) {
      struct TextExtent te;
      TextExtent(ui_rastport, (children[i])->label, strlen((children[i])->label), &te);
      original[i] = te.te_Width;

      total_original += te.te_Width;
    }

    if (total_original <= total_width) {
      // Case 1: All tabs fit — distribute remaining space equally
      WORD extra = total_width - total_original;
      WORD per_tab_extra = extra / count;
      WORD remainder = extra % count;

      for (i = 0; i < count; ++i) {
        result[i] = original[i] + per_tab_extra + (i < remainder ? 1 : 0);
      }
      return;
    }

    // Case 2: Not all fit — preserve large tabs, shrink smaller ones equally
    // First pass: identify shrinkable tabs
    average = total_width / count;

    for (i = 0; i < count; ++i) {
      if (original[i] > average) {
        fixed_total += original[i];  // keep large ones as-is
      }
      else {
        shrinkable_total += original[i];
        shrinkable_count++;
      }
    }

    if (shrinkable_count == 0) {
      // Fallback: scale all proportionally
dtw_fallback:
      for (i = 0; i < count; ++i) {
        result[i] = original[i] * total_width / total_original;
      }
      return;
    }

    // Second pass: assign final widths
    remaining = total_width - fixed_total;
    per_shrink = remaining / shrinkable_count;
    remainder = remaining % shrinkable_count;

    for (i = 0; i < count; ++i) {
      if (original[i] > average) {
        result[i] = original[i];  // keep large ones
      }
      else {
        result[i] = per_shrink + (remainder-- > 0 ? 1 : 0);
      }
    }

    // Final fit check
    for (i = 0, fixed_total = 0; i < count; i++) {
      fixed_total += result[i];
      if (fixed_total > total_width) {
        goto dtw_fallback;
      }
    }

    FreeMem(original, count);
  }
}

/******************************************************************************
 * Default size method for tab selector objects.                              *
 ******************************************************************************/
VOID sizeTabSelector(struct UIObject* self)
{
  struct UIO_Group* parent_group = (struct UIO_Group*)self->parent;
  struct UIObject** children = parent_group->children;

  /**************************************************************************
   * Tab selector's size is always inherited from its parent group. So this *
   * has to be called after parent group has calculated and/or inherited    *
   * its size!!!                                                            *
   **************************************************************************/
  self->width = parent_group->width;
  distribute_tab_widths(children, parent_group->u_tab_numtabs, parent_group->width - UIOV_TAB_NOTCH, (UBYTE*)parent_group->u_tab_widths);

  if (!self->height) {
    if (parent_group->flags & UIOF_TITLED) self->height = ui_rastport->Font->tf_YSize + UIOV_TAB_SELECTED_RISE + UIOV_TAB_MARGIN_TOP + UIOV_TAB_MARGIN_BOTTOM;
    if (parent_group->flags & UIOF_FRAMED) self->height += UIOV_TAB_FRAME_HEIGHT;
  }
}
#endif // UI_USE_TABBED_GROUPS
///
///sizeRectangle(self)
/******************************************************************************
 * Default size method for rectangle objects.                                 *
 ******************************************************************************/
#ifdef UI_USE_RECTANGLES
VOID sizeRectangle(struct UIObject* self)
{
  if (!self->width) {
    self->flags |= UIOF_INHERIT_WIDTH;
  }

  if (!self->height) {
    self->flags |= UIOF_INHERIT_HEIGHT;
  }
}
#endif // UI_USE_RECTANGLE
///
///sizeSeparator(self)
/******************************************************************************
 * Default size method for separator objects.                                 *
 ******************************************************************************/
#ifdef UI_USE_SEPARATORS
VOID sizeSeparator(struct UIObject* self)
{
  if (self->flags & UIOF_HORIZONTAL) {
    if (!self->width) {
      self->flags |= UIOF_INHERIT_WIDTH;
    }

    if (!self->height) {
      if (self->flags & FS_STRING) self->height = 1;
      else self->height = 0;
    }
  }
  else { // UIOF_VERTICAL
    if (!self->width) {
      if (self->flags & FS_STRING) self->width = 1;
      else self->width = 0;
    }

    if (!self->height) {
      self->flags |= UIOF_INHERIT_HEIGHT;
    }
  }
}
#endif // UI_USE_SEPARATORS
///
///sizeLabel(self)
/******************************************************************************
 * Default size method for label objects.                                     *
 ******************************************************************************/
#ifdef UI_USE_LABELS
VOID sizeLabel(struct UIObject* self)
{
  struct TextExtent te;

  if (!self->width) {
    TextExtent(ui_rastport, self->label, strlen(self->label), &te);
    self->width = te.te_Width;
  }

  if (!self->height) {
    self->height = ui_rastport->Font->tf_YSize;
  }
}
#endif // UI_USE_LABELS
///
///sizeButton(self)
/******************************************************************************
 * Default size method for button objects.                                    *
 ******************************************************************************/
VOID sizeButton(struct UIObject* self)
{
  struct TextExtent te;

  if (!self->width) {
    TextExtent(ui_rastport, self->label, strlen(self->label), &te);
    self->width = te.te_Width + (UIOV_BUTTON_FRAME_WIDTH + UIOV_BUTTON_MARGIN_SIDE) * 2;
  }

  if (!self->height) {
    self->height = ui_rastport->Font->tf_YSize + UIOV_BUTTON_FRAME_HEIGHT * 2 + UIOV_BUTTON_MARGIN_TOP + UIOV_BUTTON_MARGIN_BOTTOM;
  }
}
///
///sizeCheckbox(self)
/******************************************************************************
 * Default size method for checkbox objects.                                  *
 ******************************************************************************/
#ifdef UI_USE_CHECKBOXES
VOID sizeCheckbox(struct UIObject* self)
{
  struct UIO_Group* parent = (struct UIO_Group*)self->parent;
  struct TextExtent te;

  if (!self->height) {
    self->height = ui_rastport->Font->tf_YSize;
  }

  if (!self->width) {
    if (self->label) {
      TextExtent(ui_rastport, self->label, strlen(self->label), &te);
                  //Text width    gap                 tick box width
      self->width = te.te_Width + parent->child_gap + self->height;
    }
    else self->width = self->height;
  }
}
#endif //UI_USE_CHECKBOXES
///
///sizeCyclebox(self)
/******************************************************************************
 * Default size method for cyclebox objects.                                  *
 ******************************************************************************/
#ifdef UI_USE_CYCLEBOXES
VOID sizeCyclebox(struct UIObject* self)
{
  struct UIO_Cyclebox* cyclebox = (struct UIO_Cyclebox*)self;
  struct TextExtent te;

  if (!self->height) {
    self->height = ui_rastport->Font->tf_YSize + UIOV_BUTTON_FRAME_HEIGHT * 2 + UIOV_BUTTON_MARGIN_TOP + UIOV_BUTTON_MARGIN_BOTTOM;
  }

  if (!self->width) {
    STRPTR* label = cyclebox->options;
    ULONG label_width = 0;
    ULONG cycle_image_width = 0;
    ULONG i;

    cycle_image_width = self->height - (UIOV_BUTTON_FRAME_WIDTH + UIOV_BUTTON_MARGIN_SIDE) * 2 + 1;

    // Get the text width of the longest option label
    for (i = 0; i < cyclebox->num_options; i++, label++) {
      TextExtent(ui_rastport, *label, strlen(*label), &te);

      if (te.te_Width > label_width) label_width = te.te_Width;
    }

    self->width = (UIOV_BUTTON_FRAME_WIDTH + UIOV_BUTTON_MARGIN_SIDE) * 2 + cycle_image_width + UIOV_CYCLEBOX_SEPARATOR_WIDTH + label_width;
  }
}
#endif // UI_USE_CYCLEBOXES
///
///sizeString(self)
/******************************************************************************
 * Default size method for string objects.                                    *
 ******************************************************************************/
#ifdef UI_USE_STRING_GADGETS
VOID sizeString(struct UIObject* self)
{
  struct UIO_String* string = (struct UIO_String*)self;
  struct TextExtent te;

  if (!self->width) {
    //This is a suitable place to initialize string->content
    if (string->initial) {
      strncpy(string->content, string->initial, string->max_length);
      string->content_length = strlen(string->content);
    }

    TextExtent(ui_rastport, string->content, strlen(string->content), &te);
    self->width = te.te_Width + (UIOV_STRING_FRAME_WIDTH + UIOV_STRING_MARGIN_SIDE) * 2;
  }

  if (!self->height) {
    self->height = ui_rastport->Font->tf_YSize + UIOV_STRING_FRAME_HEIGHT * 2 + UIOV_STRING_MARGIN_TOP + UIOV_STRING_MARGIN_BOTTOM;
  }
}
#endif // UI_USE_STRING_GADGETS
///

/*************************
 * RENDERERS             *
 *************************/
///drawBox(x1, y1, x2, y2, pen)
/******************************************************************************
 * Draws an empty rectangle with the given pen color.                         *
 ******************************************************************************/
VOID drawBox(WORD x1, WORD y1, WORD x2, WORD y2, ULONG pen)
{
  SetAPen(ui_rastport, pen);
  Move(ui_rastport, x1, y1);
  Draw(ui_rastport, x2, y1);
  Draw(ui_rastport, x2, y2);
  Draw(ui_rastport, x1, y2);
  Draw(ui_rastport, x1, y1);
}
///
///drawFrame(x1, y1, x2, y2, style)
/******************************************************************************
 * Draws a beveled box with the requested style.                              *
 * NOTE: style flags is compatible with UIOF_ flags                           *
 ******************************************************************************/
VOID drawFrame(WORD x1, WORD y1, WORD x2, WORD y2, ULONG style)
{
  ULONG pen_shine;
  ULONG pen_shadow;

  if (style & FS_PRESSED) {
    pen_shine = UIPEN_SHADOW;
    pen_shadow = UIPEN_SHINE;
  }
  else {
    pen_shine = UIPEN_SHINE;
    pen_shadow = UIPEN_SHADOW;
  }

  switch (style & 0xF000) {
    case FS_STRING:
      Move(ui_rastport, x2, y1 + 1);
      SetAPen(ui_rastport, pen_shadow);
      Draw(ui_rastport, x2, y2);
      Draw(ui_rastport, x1 + 1, y2);
      Move(ui_rastport, x1, y2);
      SetAPen(ui_rastport, pen_shine);
      Draw(ui_rastport, x1, y1);
      Draw(ui_rastport, x2, y1);
      x1++; y1++; x2--; y2--;
      Move(ui_rastport, x2, y1 + 1);
      SetAPen(ui_rastport, pen_shine);
      Draw(ui_rastport, x2, y2);
      Draw(ui_rastport, x1 + 1, y2);
      Move(ui_rastport, x1, y2);
      SetAPen(ui_rastport, pen_shadow);
      Draw(ui_rastport, x1, y1);
      Draw(ui_rastport, x2, y1);
    break;
    case FS_BUTTON:
      Move(ui_rastport, x2, y1);
      Move(ui_rastport, x2, y1 + 1);
      SetAPen(ui_rastport, pen_shadow);
      Draw(ui_rastport, x2, y2);
      Draw(ui_rastport, x1 + 1, y2);
      Move(ui_rastport, x1, y2);
      SetAPen(ui_rastport, pen_shine);
      Draw(ui_rastport, x1, y1);
      Draw(ui_rastport, x2, y1);
    break;
  }
}
///
///drawTitledFrame(x1, y1, x2, y2, style)
/******************************************************************************
 * Draws titled group frames.                                                 *
 * NOTE: style flags is compatible with UIOF_ flags                           *
 ******************************************************************************/
VOID drawTitledFrame(STRPTR title, WORD x1, WORD y1, WORD x2, WORD y2, ULONG style)
{
  struct TextExtent te;
  ULONG letters = TextFit(ui_rastport, title, strlen(title), &te, NULL, 1, x2 - x1 - UIOV_GROUP_FRAME_TEXT * 2, ui_rastport->Font->tf_YSize);

  drawFrame(x1, y1 + ui_rastport->Font->tf_YSize / 2, x2, y2, style);
  Move(ui_rastport, x1 + UIOV_GROUP_FRAME_TEXT, y1 + ui_rastport->Font->tf_Baseline);
  ui_rastport->DrawMode = JAM2;
  SetAPen(ui_rastport, UIPEN_TEXT);
  SetBPen(ui_rastport, UIPEN_BACKGROUND);
  Text(ui_rastport, title, letters);
  ui_rastport->DrawMode = JAM1;
}
///
///drawTabFrame(x1, y1, x2, y2)
/******************************************************************************
 * Draws the left, top and right lines of a tab in a tab selector object.     *
 ******************************************************************************/
#ifdef UI_USE_TABBED_GROUPS
VOID drawTabFrame(WORD x1, WORD y1, WORD x2, WORD y2)
{
  Move(ui_rastport, x1, y2);
  SetAPen(ui_rastport, UIPEN_SHINE);
  Draw(ui_rastport, x1, y1);
  Draw(ui_rastport, x2, y1);
  SetAPen(ui_rastport, UIPEN_SHADOW);
  Draw(ui_rastport, x2, y2);
}
#endif // UI_USE_TABBED_GROUPS
///
///drawDisabledPattern(x1, y1, x2, y2)
/******************************************************************************
 * Draws a semi-transparent on/off pattern for disabled objects.              *
 ******************************************************************************/
VOID drawDisabledPattern(WORD x1, WORD y1, WORD x2, WORD y2)
{
  UWORD* pattern;
  if (x1 & 0x0001) pattern = ui_disabled_pattern1;
  else pattern = ui_disabled_pattern2;
  if (y1 & 0x0001) {
    if (pattern == ui_disabled_pattern1) pattern = ui_disabled_pattern2;
    else pattern = ui_disabled_pattern1;
  }

  SetAfPt(ui_rastport, pattern, 1);

  SetAPen(ui_rastport, UIPEN_SHADOW);
  RectFill(ui_rastport, x1, y1, x2, y2);
  SetAfPt(ui_rastport, NULL, 0);
}
///
///drawTick(x1, y1, size)
/******************************************************************************
 * Draws a scaled checkbox image.                                             *
 ******************************************************************************/
#ifdef UI_USE_CHECKBOXES
VOID drawTick(WORD x1, WORD y1, WORD size)
{
  WORD width_original = 14;
  WORD height_original = 14;
  // Scale line thicknesses
  WORD line1_width;
  WORD line2_width;
  struct {
    UBYTE x;
    UBYTE y;
  }ui_checkbox_vertices[] = {{2, 7}, {5, 10}, {11, 4}};
  struct {
    WORD x;
    WORD y;
  }vertices[3];
  ULONG i;
  UWORD shorten = 0;

  //Filter bad sizes
  switch (size) {
    case 7:
      y1--; size = 9;
    break;
    case 8:
      size = 9;
    break;
    case 9:
      y1++;
    break;
    case 10:
      x1++; y1++; size = 9;
    break;
    case 11:
    case 12:
    case 13:
      x1--; y1--; size = 14;
    break;
    case 17:
    case 18:
      size = 19;
    break;
    case 19:
      x1++;
    break;
    case 20:
      x1++; y1++; size = 19;
    break;
    case 21:
      x1--; size = 23;
    break;
    case 22:
      size = 23;
    break;
    case 24:
      x1--;
    case 25:
    case 26:
      size = 27;
    break;
    case 31:
    case 32:
      size = 33;
    break;
    case 34:
    case 35:
      x1--;
    case 36:
      x1--;
      size = 37;
    break;
    case 38:
      size = 37;
    break;
    case 39:
    case 40:
      size = 41;
    break;
    case 45:
    case 46:
      size = 44;
    break;
    case 47:
      x1--;
    case 48:
      x1--;
    case 49:
      x1--;
    case 50:
      size = 51;
    break;
  }

  line1_width = 3 * size / width_original;
  line2_width = 2 * size / width_original;
  if (line1_width < 2) line1_width = 2;

  //Scale 3 vertices:
  vertices[0].x = ui_checkbox_vertices[0].x * size / width_original;
  vertices[0].y = ui_checkbox_vertices[0].y * size / height_original;
  vertices[1].x = ui_checkbox_vertices[1].x * size / width_original;
  vertices[1].y = ui_checkbox_vertices[1].y * size / height_original;
  vertices[2].x = ui_checkbox_vertices[2].x * size / width_original;
  vertices[2].y = ui_checkbox_vertices[2].y * size / height_original;

  //Draw scaled tick
  //****************
  //Draw first line
  SetAPen(ui_rastport, UIPEN_TEXT);
  for (i = 0; i < line1_width; i++, x1++) {
    if (i > line1_width - line2_width) shorten++;
    Move(ui_rastport, x1 + vertices[0].x, y1 + vertices[0].y);
    Draw(ui_rastport, x1 + vertices[1].x - shorten, y1 + vertices[1].y - shorten);
  }
  x1 -= line1_width;

  //Draw second line
  SetAPen(ui_rastport, UIPEN_TEXT);
  for (i = 0; i < line2_width; i++, x1++) {
    Move(ui_rastport, x1 + vertices[1].x, y1 + vertices[1].y);
    Draw(ui_rastport, x1 + vertices[2].x, y1 + vertices[2].y);
  }
}
#endif // UI_USE_CHECKBOXES
///
///drawCycle(x1, y1, size, width)
/******************************************************************************
 * Draws a scaled cycle image.                                                *
 ******************************************************************************/
#ifdef UI_USE_CYCLEBOXES
VOID drawCycle(WORD x1, WORD y1, WORD size)
{
  static const UWORD p_cycle_small1[7] = {0x3C00, 0x4200, 0x8700, 0x8200, 0x8000, 0x4200, 0x3C00};
  static const UWORD p_cycle_small2[8] = {0x3C00, 0x4200, 0x8700, 0x8200, 0x8000, 0x8000, 0x4200, 0x3C00};
  static const UWORD p_cycle_mid[12] = {0x0F00, 0x3FC0, 0x70E0, 0x6060, 0xC1F8, 0xC0F0, 0xC060, 0xC000, 0x6060, 0x70E0, 0x3FC0, 0x0F00};
  static const ULONG p_cycle_big[18] =
  {0x03F00000, 0x0FFC0000, 0x1E1E0000, 0x38070000, 0x70038000, 0x60018000, 0xE0018000, 0xC00FF000, 0xC007E000,
  0xC003C000, 0xC0018000, 0xE0000000, 0x60000000, 0x70030000, 0x38070000, 0x1E1E0000, 0x0FFC0000, 0x03F00000};
  static struct BitMap bm_cycle_small1 = {2, 7, 0, 1, 0, {(PLANEPTR)p_cycle_small1, 0, 0, 0, 0, 0, 0, 0}};
  static struct BitMap bm_cycle_small2 = {2, 8, 0, 1, 0, {(PLANEPTR)p_cycle_small2, 0, 0, 0, 0, 0, 0, 0}};
  static struct BitMap bm_cycle_mid = {2, 12, 0, 1, 0, {(PLANEPTR)p_cycle_mid, 0, 0, 0, 0, 0, 0, 0}};
  static struct BitMap bm_cycle_big = {4, 18, 0, 1, 0, {(PLANEPTR)p_cycle_big, 0, 0, 0, 0, 0, 0, 0}};
  struct RastPort rp_cycle_small1;
  struct RastPort rp_cycle_small2;
  struct RastPort rp_cycle_mid;
  struct RastPort rp_cycle_big;
  UBYTE minterm = 0x60;

  InitRastPort(&rp_cycle_small1);
  InitRastPort(&rp_cycle_small2);
  InitRastPort(&rp_cycle_mid);
  InitRastPort(&rp_cycle_big);

  rp_cycle_small1.BitMap = &bm_cycle_small1;
  rp_cycle_small2.BitMap = &bm_cycle_small2;
  rp_cycle_mid.BitMap = &bm_cycle_mid;
  rp_cycle_big.BitMap = &bm_cycle_big;

  y1++;

  if (size < 10) {
    if (size == 9) x1++;
    ClipBlit(&rp_cycle_small1, 0, 0, ui_rastport, x1 + CENTER(8, size), y1 + CENTER(7, size), 8, 7, minterm);
  }
  else if (size < 14) {
    if (size & 0x0001) x1++;
    ClipBlit(&rp_cycle_small2, 0, 0, ui_rastport, x1 + CENTER(8, size), y1 + CENTER(8, size), 8, 8, minterm);
  }
  else if (size < 22) {
    x1++;
    ClipBlit(&rp_cycle_mid, 0, 0, ui_rastport, x1 + CENTER(13, size), y1 + CENTER(12, size), 13, 12, minterm);
  }
  else {
    x1++;
    ClipBlit(&rp_cycle_big, 0, 0, ui_rastport, x1 + CENTER(20, size), y1 + CENTER(18, size), 20, 18, minterm);
  }
}
#endif // UI_USE_CYCLEBOXES
///

///drawGroup(self)
/******************************************************************************
 * Default draw method for group objects which renders the group onto the     *
 * rastport including all its children.                                       *
 ******************************************************************************/
VOID drawGroup(struct UIObject* self)
{
  struct UIO_Group* group = (struct UIO_Group*)self;
  struct UIObject** child = group->children;
  struct UIObject* obj;
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD x2 = x + self->width;
  WORD y2 = y + self->height;

  setClip(self->parent);

 /**********************************************************************
  * Implement here how to clear your group's background (x, y, x2, y2) *
  **********************************************************************/
  SetAPen(ui_rastport, UIPEN_BACKGROUND);
  RectFill(ui_rastport, x, y, x2, y2);

#ifdef UI_USE_TABBED_GROUPS
  if (self->flags & UIOF_TABBED) {
    if (self->flags & UIOF_FRAMED) drawFrame(x, y + group->u_tab_selector->height - UIOV_TAB_FRAME_HEIGHT, x2, y2, self->flags);
    group->u_tab_selector->draw(group->u_tab_selector);
    obj = child[group->u_tab_selected];
    obj->draw(obj);
  }
  else
#endif // UI_USE_TABBED_GROUPS
  {
    //Content area of the group
    WORD c_x1 = group->scroll.x;
    WORD c_y1 = group->scroll.y;
    WORD c_x2 = c_x1 + group->width - group->margin.left - group->margin.right;
    WORD c_y2 = c_y1 + group->height - group->margin.top - group->margin.bottom;

    if (self->flags & UIOF_FRAMED) {
      if (self->flags & UIOF_TITLED) drawTitledFrame(self->label, x, y, x2, y2, self->flags);
      else drawFrame(x, y, x2, y2, self->flags);
    }

#ifdef UI_USE_SCROLLBARS
    if (group->scroll.bar_horiz) {
      group->scroll.bar_horiz->draw(group->scroll.bar_horiz);
      c_y2 -= UIOV_SCROLLBAR_HEIGHT + group->child_gap;
    }
    if (group->scroll.bar_vert) {
      group->scroll.bar_vert->draw(group->scroll.bar_vert);
      c_x2 -= UIOV_SCROLLBAR_HEIGHT + group->child_gap;
    }
#endif // UI_USE_SCROLLBARS

    //Draw the children of the group
    while ((obj = *child++)) {
      // Draw only objects that fall into the visible content area
      if (obj->x + obj->width >= c_x1 && obj->x < c_x2 && obj->y + obj->height >= c_y1 && obj->y < c_y2) {
        if (obj->draw) obj->draw(obj);
      }
    }
  }

  if (self->flags & UIOF_DISABLED) {
    setClip(self->parent);
#ifdef UI_USE_TABBED_GROUPS
    if (self->flags & UIOF_TABBED)
      drawDisabledPattern(x, y, x + group->width, y + group->height);
    else
#endif // UI_USE_TABBED_GROUPS
      drawDisabledPattern(x + group->margin.left, y + group->margin.top, x + group->width - group->margin.right, y + group->height - group->margin.bottom);
  }
}
///
///drawTabSelector(self)
/******************************************************************************
 * Default draw method for tab selector objects. Renders the tab selector     *
 * onto the rastport.                                                         *
 ******************************************************************************/
#ifdef UI_USE_TABBED_GROUPS
VOID drawTabSelector(struct UIObject* self)
{
  struct UIO_Group* group = (struct UIO_Group*)self->parent;
  WORD x1_obj = inheritX(self->parent); //NOTE: We get the X & Y of the parent group
  WORD y1_obj = inheritY(self->parent); //to get rid of the parent group's margins.
  WORD x2_obj = x1_obj + self->width;
  WORD y2_obj = y1_obj + self->height - UIOV_TAB_FRAME_HEIGHT;

  setClip(group->parent);

 /**********
  * RENDER *
  **********/
  {
    WORD x1 = x1_obj + UIOV_TAB_NOTCH;
    WORD y1 = y1_obj + UIOV_TAB_SELECTED_RISE;
    WORD x2; // Calculated below per tab
    WORD y2 = y2_obj - UIOV_TAB_FRAME_HEIGHT;
    WORD selected_tab_x1 = 0;
    UBYTE* tab_widths = (UBYTE*)group->u_tab_widths;
    UWORD num_tabs = group->u_tab_numtabs;
    UWORD selected = group->u_tab_selected;
    UWORD tab = 0;
    UWORD frame_margin = 0;

    //TEMP TODO WARNING
    //Clear Background
    SetAPen(ui_rastport, UIPEN_BACKGROUND);
    RectFill(ui_rastport, x1_obj, y1_obj, x2_obj, y2);

    //Render tab frames
    if (self->flags & UIOF_FRAMED) {
      frame_margin = (UIOV_TAB_FRAME_WIDTH + UIOV_TAB_MARGIN_SIDE);

      //Render unselected tabs
      for (tab = 0; tab < num_tabs; tab++) {
        if (tab == selected) selected_tab_x1 = x1 - UIOV_TAB_FRAME_WIDTH;
        else {
          if (tab == num_tabs - 1) x2 = x2_obj - UIOV_TAB_NOTCH;
          else x2 = x1 + tab_widths[tab] - 1;

         /********************************************************
          * Render your stylized tab frame here (x1, y1, x2, y2) *
          ********************************************************/
          drawTabFrame(x1, y1, x2, y2);
        }
        x1 += tab_widths[tab];
      }

      //Render selected tab
      if (selected == num_tabs - 1) x2 = x2_obj;
      else x2 = selected_tab_x1 + tab_widths[selected] + UIOV_TAB_FRAME_WIDTH * 2 - 1;

     /*****************************************************************************
      * Render your SELECTED tab frame here (selected_tab_x1, y1_obj, x2, y2_obj) *
      *****************************************************************************/
      drawTabFrame(selected_tab_x1, y1_obj, x2, y2_obj);
      //Clean the frame section under selected tab
      SetAPen(ui_rastport, UIPEN_BACKGROUND);
      Move(ui_rastport, selected_tab_x1 + UIOV_TAB_FRAME_WIDTH, y2_obj);
      Draw(ui_rastport, x2 - UIOV_TAB_FRAME_WIDTH, y2_obj);
    }

    //Render tab labels
    if (self->flags & UIOF_TITLED) {
      struct UIObject** child = group->children;
      struct UIObject* obj;
      struct TextExtent te;
      ULONG letters;
      UWORD x2;
      UWORD tab_inner_width;
      selected_tab_x1 = 0;

      SetAPen(ui_rastport, UIPEN_TEXT);

      //Render unselected labels
      for (tab = 0, x1 = x1_obj + UIOV_TAB_NOTCH; tab < num_tabs; tab++) {
        obj = *child++;

        if (tab == selected) selected_tab_x1 = x1 - 1;
        else {
          if (tab == num_tabs - 1) x2 = x2_obj - UIOV_TAB_NOTCH;
          else x2 = x1 + tab_widths[tab] - 1;
          tab_inner_width = x2 - x1 - frame_margin * 2;

         /*****************************************************************
          * Render your stylized tab label here (x1, y1, tab_inner_width) *
          *****************************************************************/
          letters = TextFit(ui_rastport, obj->label, strlen(obj->label), &te, NULL, 1, tab_inner_width, self->height);
          Move(ui_rastport, x1 + frame_margin + CENTER(te.te_Width, tab_inner_width), y1 + UIOV_TAB_MARGIN_TOP + ui_rastport->Font->tf_Baseline);
          Text(ui_rastport, obj->label, letters);
        }

        x1 += tab_widths[tab];
      }

      //Render selected label
      obj = group->children[selected];
      if (selected == num_tabs - 1) x2 = x2_obj;
      else x2 = selected_tab_x1 + tab_widths[selected];
      tab_inner_width = x2 - selected_tab_x1 - frame_margin * 2;

     /******************************************************************************
      * Render your SELECTED tab label here (selected_tab_x1, y1, tab_inner_width) *
      ******************************************************************************/
      letters = TextFit(ui_rastport, obj->label, strlen(obj->label), &te, NULL, 1, tab_inner_width, self->height);
      Move(ui_rastport, selected_tab_x1 + frame_margin + CENTER(te.te_Width, tab_inner_width), y1 + UIOV_TAB_MARGIN_TOP - UIOV_TAB_SELECTED_RISE + ui_rastport->Font->tf_Baseline);
      Text(ui_rastport, obj->label, letters);
    }
  }
}
#endif // UI_USE_TABBED_GROUPS
///
///drawScrollbar(self)
/******************************************************************************
 * Default draw method for scrollbar objects. Renders the scrollbar onto the  *
 * rastport.                                                                  *
 ******************************************************************************/
#ifdef UI_USE_SCROLLBARS
VOID drawScrollbar(struct UIObject* self)
{
  struct UIO_Group* group = (struct UIO_Group*)self->parent;
  WORD x1_obj = inheritX(self) + group->scroll.x;
  WORD y1_obj = inheritY(self) + group->scroll.y;
  WORD x2_obj = x1_obj + self->width;
  WORD y2_obj = y1_obj + self->height;
  ULONG bar_pen = self->flags & UIOF_HOVERED ? UIOV_SCROLLBAR_ACTIVE_COLOR : UIOV_SCROLLBAR_BAR_COLOR;

  setClip(group->parent);

  if (self->flags & UIOF_HORIZONTAL)
  {
    WORD content_width = group->width - (group->margin.left + group->margin.right + (group->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + group->child_gap) : 0));
    UWORD bar_width = self->width * content_width / group->scroll.width;
    UWORD bar_pos = self->width * group->scroll.x / group->scroll.width;
    LONG x1_bar;
    LONG x2_bar;
    BOOL rightmost = FALSE;

    if (group->scroll.x + content_width >= group->scroll.width) {
      bar_pos = self->width - bar_width;
      rightmost = TRUE;
    }
    x1_bar = x1_obj + bar_pos;
    x2_bar = x1_bar + bar_width;

    /******************************************************
     * Implement your stylized horizontal scrollbar below *
     ******************************************************/
    if (bar_width) {
      if (bar_pos) {
        // Draw "PAGE LEFT" area (x1_obj, y1_obj, x1_bar, y2_obj)
        SetAPen(ui_rastport, UIOV_SCROLLBAR_PAGE_COLOR);
        RectFill(ui_rastport, x1_obj, y1_obj, x1_bar, y2_obj);
      }
      // Draw "BAR" area (x1_bar, y1_obj, x2_bar, y2_obj)
      SetAPen(ui_rastport, bar_pen);
      RectFill(ui_rastport, x1_bar, y1_obj, x2_bar, y2_obj);
      // Draw "PAGE RIGHT" area (x2_bar, y1_obj, x2_obj, y2_obj)
      if (!rightmost) {
        SetAPen(ui_rastport, UIOV_SCROLLBAR_PAGE_COLOR);
        RectFill(ui_rastport, x2_bar, y1_obj, x2_obj, y2_obj);
      }
    }
  }
  else {
    WORD content_height = group->height - (group->margin.top + group->margin.bottom + (group->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + group->child_gap) : 0));
    UWORD bar_height = self->height * content_height / group->scroll.height;
    UWORD bar_pos = self->height * group->scroll.y / group->scroll.height;
    WORD y1_bar;
    WORD y2_bar;
    BOOL bottommost = FALSE;

    if (group->scroll.y + content_height >= group->scroll.height) {
      bar_pos = self->height - bar_height;
      bottommost = TRUE;
    }

    y1_bar = y1_obj + bar_pos;
    y2_bar = y1_bar + bar_height;

    /****************************************************
     * Implement your stylized vertical scrollbar below *
     ****************************************************/
    if (bar_height) {
      if (bar_pos) {
        // Draw "PAGE UP" area (x1_obj, y1_obj, x2_obj, y1_bar)
        SetAPen(ui_rastport, UIOV_SCROLLBAR_PAGE_COLOR);
        RectFill(ui_rastport, x1_obj, y1_obj, x2_obj, y1_bar);
      }
      // Draw "BAR" area (x1_obj, y1_bar, x2_obj, y2_bar)
      SetAPen(ui_rastport, bar_pen);
      RectFill(ui_rastport, x1_obj, y1_bar, x2_obj, y2_bar);
      // Draw "PAGE DOWN" area (x1_obj, y2_bar, x2_obj, y2_obj)
      if (!bottommost) {
        SetAPen(ui_rastport, UIOV_SCROLLBAR_PAGE_COLOR);
        RectFill(ui_rastport, x1_obj, y2_bar, x2_obj, y2_obj);
      }
    }
  }
}
#endif // UI_USE_SCROLLBARS
///

///drawSeparator(self)
/******************************************************************************
 * Default draw method for separator objects. Renders the separator onto the  *
 * rastport.                                                                  *
 ******************************************************************************/
#ifdef UI_USE_SEPARATORS
VOID drawSeparator(struct UIObject* self)
{
  WORD x1 = inheritX(self);
  WORD y1 = inheritY(self);
  WORD x2 = x1 + self->width;
  WORD y2 = y1 + self->height;

  setClip(self->parent);

 /**************************************************
  * Implement your stylized separator object below *
  *************************************************/
  if (self->flags & FS_STRING) {
    if (self->flags & UIOF_HORIZONTAL) {
      WORD y = y1 + (self->height - 1) / 2;
      SetAPen(ui_rastport, UIPEN_SHINE);
      Move(ui_rastport, x1, y);
      Draw(ui_rastport, x2, y++);
      SetAPen(ui_rastport, UIPEN_SHADOW);
      Move(ui_rastport, x1, y);
      Draw(ui_rastport, x2, y);
    }
    else { // UIOF_VERTICAL
      WORD x = x1 + (self->width - 1) / 2;
      SetAPen(ui_rastport, UIPEN_SHINE);
      Move(ui_rastport, x, y1);
      Draw(ui_rastport, x++, y2);
      SetAPen(ui_rastport, UIPEN_SHADOW);
      Move(ui_rastport, x, y1);
      Draw(ui_rastport, x, y2);
    }
  }
  else { // FS_BUTTON
    if (self->flags & UIOF_HORIZONTAL) {
      WORD y = y1 + self->height / 2;
      SetAPen(ui_rastport, UIPEN_SHADOW);
      Move(ui_rastport, x1, y);
      Draw(ui_rastport, x2, y);
    }
    else { // UIOF_VERTICAL
      WORD x = x1 + self->width / 2;
      SetAPen(ui_rastport, UIPEN_SHADOW);
      Move(ui_rastport, x, y1);
      Draw(ui_rastport, x, y2);
    }
  }
}
#endif // UI_USE_SEPARATORS
///
///drawLabel(self)
/******************************************************************************
 * Default draw method for label objects. Renders the label onto the          *
 * rastport.                                                                  *
 ******************************************************************************/
#ifdef UI_USE_LABELS
VOID drawLabel(struct UIObject* self)
{
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD text_x;
  WORD text_y;
  struct TextExtent te;
  ULONG chars;

  setClip(self->parent);

 /***********************************************
  * Implement your stylized label object below  *
  ***********************************************/
  if (self->label && *self->label) {
    chars = TextFit(ui_rastport, self->label, strlen(self->label), &te, NULL, 1, self->width, ui_rastport->Font->tf_YSize);
    if (self->flags & UIOF_ALIGN_CENTER) {
      text_x = x + CENTER(te.te_Width, self->width);
    }
    else if (self->flags & UIOF_ALIGN_RIGHT) {
      text_x = x + self->width - te.te_Width;
    }
    else {
      text_x = x;
    }

    if (self->flags & UIOF_ALIGN_CENTER_V) {
      text_y = y + CENTER(te.te_Height, self->height) + ui_rastport->Font->tf_Baseline;
    }
    else if (self->flags & UIOF_ALIGN_BOTTOM) {
      text_y = y + self->height - te.te_Height + ui_rastport->Font->tf_Baseline;
    }
    else {
      text_y = y + ui_rastport->Font->tf_Baseline;
    }

    Move(ui_rastport, text_x, text_y + self->u_label_y_offset);
    SetAPen(ui_rastport, UIPEN_TEXT);
    Text(ui_rastport, self->label, chars);
  }
}
#endif // UI_USE_LABELS
///
///drawButton(self)
/******************************************************************************
 * Default draw method for button objects. Renders the button onto the        *
 * rastport.                                                                  *
 ******************************************************************************/
VOID drawButton(struct UIObject* self)
{
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD x2 = x + self->width;
  WORD y2 = y + self->height;
  WORD text_y = y + UIOV_BUTTON_MARGIN_TOP + CENTER(ui_rastport->Font->tf_YSize, self->height - UIOV_BUTTON_MARGIN_TOP - UIOV_BUTTON_MARGIN_BOTTOM) + ui_rastport->Font->tf_Baseline;
  struct TextExtent te;
  ULONG fill_pen = self->flags & (UIOF_HOVERED | UIOF_SELECTED) && !(self->flags & UIOF_DISABLED) ? UIPEN_HALF_SHINE : UIPEN_BACKGROUND;

  setClip(self->parent);

 /***********************************************
  * Implement your stylized button object below *
  ***********************************************/
  SetAPen(ui_rastport, fill_pen);
  RectFill(ui_rastport, x, y, x2, y2);

  if (self->flags & UIOF_FRAMED) drawFrame(x, y, x + self->width, y + self->height, self->flags);
  TextExtent(ui_rastport, self->label, strlen(self->label), &te);
  Move(ui_rastport, x + CENTER(te.te_Width, self->width), text_y);
  SetAPen(ui_rastport, UIPEN_TEXT);
  Text(ui_rastport, self->label, strlen(self->label));

  if (self->flags & UIOF_DISABLED) {
    drawDisabledPattern(x + UIOV_BUTTON_FRAME_WIDTH,
                        y + UIOV_BUTTON_FRAME_HEIGHT,
                        x + self->width - UIOV_BUTTON_FRAME_WIDTH,
                        y + self->height - UIOV_BUTTON_FRAME_HEIGHT);
  }
}
///
///drawCheckbox(self)
/******************************************************************************
 * Default draw method for checkbox objects. Renders the button onto the    *
 * rastport.                                                                  *
 ******************************************************************************/
#ifdef UI_USE_CHECKBOXES
VOID drawCheckbox(struct UIObject* self)
{
  struct UIO_Group* parent = (struct UIO_Group*)self->parent;
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  struct TextExtent te;
  WORD tick_box_width = self->height;
  ULONG fill_pen = self->flags & (UIOF_HOVERED | UIOF_SELECTED) && !(self->flags & UIOF_DISABLED) ? UIPEN_HALF_SHINE : UIPEN_BACKGROUND;

  setClip(self->parent);

 /*************************************************
  * Implement your stylized checkbox object below *
  *************************************************/
  if (self->flags & UIOF_FRAMED) drawFrame(x, y, x + tick_box_width, y + self->height, self->flags & ~UIOF_PRESSED);
  //Clear autton area
  {
    WORD x1 = x + UIOV_BUTTON_FRAME_WIDTH;
    WORD y1 = y + UIOV_BUTTON_FRAME_HEIGHT;
    WORD x2 = x + tick_box_width - UIOV_BUTTON_FRAME_WIDTH;
    WORD y2 = y + self->height - UIOV_BUTTON_FRAME_HEIGHT;

    SetAPen(ui_rastport, fill_pen);
    RectFill(ui_rastport, x1, y1, x2, y2);
  }
  if (self->flags & UIOF_PRESSED) drawTick(x, y, tick_box_width);
  if (self->label) {
    ULONG chars = TextFit(ui_rastport, self->label, strlen(self->label), &te, NULL, 1, self->width - tick_box_width - parent->child_gap, self->height);

    SetAPen(ui_rastport, UIPEN_TEXT);
    Move(ui_rastport, x + tick_box_width + parent->child_gap - 1, y + CENTER(ui_rastport->Font->tf_YSize, self->height) + ui_rastport->Font->tf_Baseline + 1);
    Text(ui_rastport, self->label, chars);
  }

  if (self->flags & UIOF_DISABLED) {
    drawDisabledPattern(x + UIOV_BUTTON_FRAME_WIDTH,
                        y + UIOV_BUTTON_FRAME_HEIGHT,
                        x + self->width - UIOV_BUTTON_FRAME_WIDTH,
                        y + self->height - UIOV_BUTTON_FRAME_HEIGHT);
  }
}
#endif // UI_USE_CHECKBOXES
///
///drawCyclebox(self)
/******************************************************************************
 * Default draw method for cyclebox objects. Renders the cycle gadget onto    *
 * the rastport.                                                              *
 ******************************************************************************/
#ifdef UI_USE_CYCLEBOXES
VOID drawCyclebox(struct UIObject* self)
{
  struct UIO_Cyclebox* cyclebox = (struct UIO_Cyclebox*)self;
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD x2 = x + self->width;
  WORD y2 = y + self->height;
  WORD cycle_image_width = self->height - (UIOV_BUTTON_FRAME_WIDTH + UIOV_BUTTON_MARGIN_SIDE) * 2 + 1;
  WORD separator_x = x + UIOV_BUTTON_FRAME_WIDTH + UIOV_BUTTON_MARGIN_SIDE + cycle_image_width;
  WORD separator_y = y + UIOV_BUTTON_FRAME_HEIGHT + 1;
  WORD separator_y2 = y + self->height - UIOV_BUTTON_FRAME_HEIGHT - 1;
  WORD text_x = separator_x + UIOV_CYCLEBOX_SEPARATOR_WIDTH;
  WORD text_y = y + UIOV_BUTTON_MARGIN_TOP + CENTER(ui_rastport->Font->tf_YSize, self->height - UIOV_BUTTON_MARGIN_TOP - UIOV_BUTTON_MARGIN_BOTTOM);
  STRPTR label = cyclebox->options[cyclebox->selected];
  ULONG fill_pen = self->flags & (UIOF_HOVERED | UIOF_SELECTED) && !(self->flags & UIOF_DISABLED) ? UIPEN_HALF_SHINE : UIPEN_BACKGROUND;

  setClip(self->parent);
  waitVBeam(y2 + ui_screen_start);

 /*****************************************************
  * Implement your stylized cycle gadget object below *
  *****************************************************/
  SetAPen(ui_rastport, fill_pen);
  RectFill(ui_rastport, x + UIOV_BUTTON_FRAME_WIDTH, y + UIOV_BUTTON_FRAME_HEIGHT, x2 - UIOV_BUTTON_FRAME_WIDTH, y2 - UIOV_BUTTON_FRAME_HEIGHT);

  if (self->flags & UIOF_FRAMED) drawFrame(x, y, x + self->width, y + self->height, self->flags);
  drawCycle(x + UIOV_BUTTON_FRAME_WIDTH, y + UIOV_BUTTON_FRAME_HEIGHT, cycle_image_width);

  //Draw separator
  SetAPen(ui_rastport, UIPEN_SHADOW);
  Move(ui_rastport, separator_x, separator_y);
  Draw(ui_rastport, separator_x, separator_y2);
  SetAPen(ui_rastport, UIPEN_SHINE);
  Move(ui_rastport, separator_x+1, separator_y);
  Draw(ui_rastport, separator_x+1, separator_y2);

  //Draw label
  Move(ui_rastport, text_x, text_y + ui_rastport->Font->tf_Baseline);
  SetAPen(ui_rastport, UIPEN_TEXT);
  Text(ui_rastport, label, strlen(label));

  if (self->flags & UIOF_DISABLED) {
    drawDisabledPattern(x + UIOV_BUTTON_FRAME_WIDTH,
                        y + UIOV_BUTTON_FRAME_HEIGHT,
                        x + self->width - UIOV_BUTTON_FRAME_WIDTH,
                        y + self->height - UIOV_BUTTON_FRAME_HEIGHT);
  }
}
#endif // UI_USE_CYCLEBOXES
///
///drawString(self)
/******************************************************************************
 * Default draw method for string objects. Renders the string object onto the *
 * rastport.                                                                  *
 ******************************************************************************/
#ifdef UI_USE_STRING_GADGETS
VOID drawString(struct UIObject* self)
{
  struct UIO_String* string = (struct UIO_String*) self;
  WORD x_obj = inheritX(self);
  WORD y_obj = inheritY(self);
  WORD x1 = x_obj + UIOV_STRING_FRAME_WIDTH + UIOV_STRING_MARGIN_SIDE;
  WORD y1 = y_obj + UIOV_STRING_FRAME_HEIGHT + UIOV_STRING_MARGIN_TOP;
  WORD x2 = x_obj + self->width - (UIOV_STRING_FRAME_WIDTH + UIOV_STRING_MARGIN_SIDE);
  WORD y2 = y_obj + self->height - (UIOV_STRING_FRAME_HEIGHT + UIOV_STRING_MARGIN_BOTTOM);
  WORD text_x = x1 - string->offset;
  WORD text_y = y1 + ui_rastport->Font->tf_Baseline;
  struct TextExtent te;
  struct Rectangle rect;
  struct Region* region;
  struct Region* old_region;
  ULONG fill_pen = self->flags & (UIOF_HOVERED | UIOF_SELECTED) && !(self->flags & UIOF_DISABLED) ? UIPEN_HALF_SHINE : UIPEN_BACKGROUND;

  /***********************************************
   * Implement your stylized string object below *
   ***********************************************/
  waitVBeam(y2 + ui_screen_start);

  if (self->flags & UIOF_FRAMED) {
    setClip(self->parent);
    drawFrame(x_obj, y_obj, x_obj + self->width, y_obj + self->height, self->flags & ~UIOF_PRESSED);
  }

  region = inheritClipRegion(self->parent);
  rect.MinX = x1;
  rect.MinY = y1;
  rect.MaxX = x2;
  rect.MaxY = y2;
  AndRectRegion(region, &rect);
  old_region = InstallClipRegion(ui_rastport->Layer, region);
  if (old_region) DisposeRegion(old_region);
  ui_current_clipper = self;

  // Clear background
  SetAPen(ui_rastport, fill_pen);
  RectFill(ui_rastport, x1, y1, x2, y2);

  if (self == ui_active_object) {
    if (self->flags & UIOF_CURSOR_ON) {
      WORD cursor_x1;
      UWORD cursor_width;
      STRPTR cursor_ch = &string->content[string->cursor_pos];
      ULONG i;

      if (string->flags & UIOF_PASSWORD) {
        //Calculate cursor width
        TextExtent(ui_rastport, ui_password_string, 1, &te);
        #if UIOV_STRING_CURSOR_WIDTH == UIOV_STRING_CURSOR_WIDTH_CHAR
        cursor_width = te.te_Width - 1;
        #else
        cursor_width = UIOV_STRING_CURSOR_WIDTH - 1;
        #endif

        //Calculate cursor coordinate
        cursor_x1 = x1 + te.te_Width * string->cursor_pos - string->offset;

        //Restrict cursor to be within object display
        if (cursor_x1 < x1) {
          WORD pixels = x1 - cursor_x1;
          string->offset -= pixels;
          text_x += pixels;
        }
        else if (cursor_x1 + cursor_width > x2) {
          WORD pixels = cursor_x1 + cursor_width - x2;
          string->offset += pixels;
          text_x -= pixels;
        }

        //Render cursor
        SetAPen(ui_rastport, UIOV_STRING_CURSOR_PEN);
        RectFill(ui_rastport, cursor_x1, y1, cursor_x1 + cursor_width, y2);

        //Render pasword characters
        Move(ui_rastport, text_x, text_y);
        SetAPen(ui_rastport, UIPEN_TEXT);
        for (i = 0; i < string->content_length; i++) {
          Text(ui_rastport, ui_password_string, 1);
        }
      }
      else {
        //Calculate cursor width
        #if UIOV_STRING_CURSOR_WIDTH == UIOV_STRING_CURSOR_WIDTH_CHAR
        TextExtent(ui_rastport, cursor_ch, 1, &te);
        cursor_width = te.te_Width - 1;
        #else
        cursor_width = UIOV_STRING_CURSOR_WIDTH - 1;
        #endif

        //Calculate cursor coordinate
        TextExtent(ui_rastport, string->content, string->cursor_pos, &te);
        cursor_x1 = x1 + te.te_Width - string->offset;

        //Restrict cursor to be within object display
        if (cursor_x1 < x1) {
          WORD pixels = x1 - cursor_x1;
          string->offset -= pixels;
          text_x += pixels;
        }
        else if (cursor_x1 + cursor_width > x2) {
          WORD pixels = cursor_x1 + cursor_width - x2;
          string->offset += pixels;
          text_x -= pixels;
        }

        //Render text upto cursor pos
        Move(ui_rastport, text_x, text_y);
        SetAPen(ui_rastport, UIPEN_TEXT);
        Text(ui_rastport, string->content, string->cursor_pos);
        cursor_x1 = ui_rastport->cp_x;
        //Render cursor
        SetAPen(ui_rastport, UIOV_STRING_CURSOR_PEN);
        RectFill(ui_rastport, cursor_x1, y1, cursor_x1 + cursor_width, y2);
        //Render string after cursor
        SetAPen(ui_rastport, UIPEN_TEXT);
        Move(ui_rastport, cursor_x1, text_y);
        Text(ui_rastport, cursor_ch, strlen(cursor_ch));
      }
    }
    else goto ds_draw_inactive_object_text;
  }
  else {
    if (!string->content_length && string->placeholder) {
      Move(ui_rastport, text_x, text_y);
      SetAPen(ui_rastport, UIOV_STRING_PLACEHOLDER_PEN);
      Text(ui_rastport, string->placeholder, strlen(string->placeholder));
    }
    else {
ds_draw_inactive_object_text:
      Move(ui_rastport, text_x, text_y);
      SetAPen(ui_rastport, UIPEN_TEXT);
      if (string->flags & UIOF_PASSWORD) {
        ULONG i;
        for (i = 0; i < string->content_length; i++)
          Text(ui_rastport, ui_password_string, 1);
      }
      else Text(ui_rastport, string->content, strlen(string->content));
    }
  }

  //Render filler dots
  if (string->flags & UIOF_DOTTED) {
    LONG i;
    LONG num_dots = string->max_length;

    if (string->content_length) num_dots -= string->content_length;
    else if (self != ui_active_object && string->placeholder) num_dots -= strlen(string->placeholder);

    for (i = 0; i < num_dots; i++) {
      Text(ui_rastport, ui_dotted_string, 1);
    }
  }

  if (self->flags & UIOF_DISABLED) {
    drawDisabledPattern(x_obj + UIOV_STRING_FRAME_WIDTH,
                        y_obj + UIOV_STRING_FRAME_HEIGHT,
                        x_obj + self->width - UIOV_STRING_FRAME_WIDTH,
                        y_obj + self->height - UIOV_STRING_FRAME_HEIGHT);
  }
}
#endif // UI_USE_STRING_GADGETS
///

/*************************
 * ACTIONS               *
 *************************/
///actionTabSelector(self, pointer_x, pointer_y, pressed)
#ifdef UI_USE_TABBED_GROUPS
/******************************************************************************
 * Default onActive method for tab selector objects. This function is         *
 * called when the tab selector object is clicked on and will change the      *
 * active tab and re-draw the tabbed group.                                   *
 ******************************************************************************/
VOID actionTabSelector(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  struct UIO_Group* group = (struct UIO_Group*)self->parent;
  UBYTE* tab_widths = (UBYTE*)group->u_tab_widths;
  WORD x1_obj = inheritX((struct UIObject*)group); //NOTE: We get the X & Y of the parent group
//WORD y1_obj = inheritY((struct UIObject*)group); //to get rid of the parent group's margins.
  WORD x2_obj = x1_obj + self->width;
//WORD y2_obj = y1_obj + self->height;
  WORD x1 = x1_obj + UIOV_TAB_NOTCH;
  WORD x2;
  UWORD selected = 0xFFFF;
  UWORD tab;

  //Locate which tab the user has clicked
  for (tab = 0; tab < group->u_tab_numtabs; tab++) {
    if (tab == group->u_tab_numtabs - 1) x2 = x2_obj - UIOV_TAB_NOTCH;
    else x2 = x1 + tab_widths[tab];
    if (pointer_x > x1 && pointer_x < x2) {
      selected = tab;
      break;
    }

    x1 += tab_widths[tab];
  }

  if (selected != 0xFFFF && selected != group->u_tab_selected) {
    group->u_tab_selected = selected;
    if (ui_selected_object) ui_selected_object->flags &= ~UIOF_SELECTED;
    ui_selected_object = NULL;
    ui_cycle_chain_index = ui_cycle_chain_size;
    group->draw((struct UIObject*)group);
  }
}
#endif // UI_USE_TABBED_GROUPS
///
///actionScrollbar(self, pointer_x, pointer_y, pressed)
/******************************************************************************
 * Default onActive method for scrollbar objects.                             *
 ******************************************************************************/
#ifdef UI_USE_SCROLLBARS
VOID actionScrollbar(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  struct UIO_Group* group = (struct UIO_Group*)self->parent;
  #define SBS_NONE      0
  #define SBS_SCROLL    1
  #define SBS_PAGE_UP   2
  #define SBS_PAGE_DOWN 3
  #define SB_PAGE_DELAY 10
  static ULONG state = SBS_NONE;
  static WORD old_pointer_x;
  static WORD old_pointer_y;
  static WORD page_delay;

  WORD x = inheritX(self) + group->scroll.x;
  WORD y = inheritY(self) + group->scroll.y;
  //WORD x2 = x + self->width;
  //WORD y2 = y + self->height;

  if (self->flags & UIOF_HORIZONTAL) {
    WORD content_width = group->width - group->margin.left - group->margin.right - (group->scroll.bar_vert ? (UIOV_SCROLLBAR_WIDTH + group->child_gap) : 0);
    UWORD bar_width = self->width * content_width / group->scroll.width;
    UWORD bar_pos = self->width * group->scroll.x / group->scroll.width;
    WORD bar_pos_max = self->width - bar_width;
    LONG x1_bar;
    LONG x2_bar;

    if (bar_pos + bar_width >= self->width) {
      bar_pos = bar_pos_max;
    }
    x1_bar = x + bar_pos;
    x2_bar = x1_bar + bar_width;

    //Test if the mouse button is still pressed
    if (pressed) {
scroll_horizontal:
      if (ui_active_object == self) {
        WORD scroll_value;

        switch (state) {
          case SBS_SCROLL:
            scroll_value = pointer_x - old_pointer_x;
            old_pointer_x = pointer_x;

            scroll_value = scroll_value * group->scroll.width / content_width;
            scrollGroupHoriz((struct UIO_Group*)self->parent, scroll_value);
          break;
          case SBS_PAGE_UP:
            if (page_delay < SB_PAGE_DELAY) {
              page_delay++;
              return;
            }
            page_delay = 0;
            scroll_value = -bar_width;

            scroll_value = scroll_value * group->scroll.width / content_width;
            scrollGroupHoriz((struct UIO_Group*)self->parent, scroll_value);
          break;
          case SBS_PAGE_DOWN:
            if (page_delay < SB_PAGE_DELAY) {
              page_delay++;
              return;
            }
            page_delay = 0;
            scroll_value = bar_width;

            scroll_value = scroll_value * group->scroll.width / content_width;
            scrollGroupHoriz((struct UIO_Group*)self->parent, scroll_value);
          break;
        }
      }
      else {
        //Check where has been clicked
        if (pointer_x < x1_bar) { // Top of the scrollbar
          ui_active_object = self;
          state = SBS_PAGE_UP;
          page_delay = SB_PAGE_DELAY;
          goto scroll_horizontal;
        }
        else if (pointer_x > x2_bar) { // Bottom of the scrollbar
          ui_active_object = self;
          state = SBS_PAGE_DOWN;
          page_delay = SB_PAGE_DELAY;
          goto scroll_horizontal;
        }
        else { // Scrollbar itself
          ui_active_object = self;
          state = SBS_SCROLL;
          old_pointer_x = pointer_x;
          self->flags |= UIOF_PRESSED;
          if (self->draw) self->draw(self);
        }
      }
    }
    else {
      if (self->flags & UIOF_PRESSED) {
        self->flags &= ~UIOF_PRESSED;
        if (self->draw) self->draw(self);
      }

      state = SBS_NONE;
      ui_active_object = NULL;
    }
  }
  else {
    WORD content_height = group->height - group->margin.top - group->margin.bottom - (group->scroll.bar_horiz ? (UIOV_SCROLLBAR_HEIGHT + group->child_gap) : 0);
    WORD bar_height = self->height * content_height / group->scroll.height;
    WORD bar_pos = self->height * group->scroll.y / group->scroll.height;
    WORD bar_pos_max = self->height - bar_height;
    LONG y1_bar;
    LONG y2_bar;

    if (bar_pos + bar_height >= self->height) {
      bar_pos = bar_pos_max;
    }

    y1_bar = y + bar_pos;
    y2_bar = y1_bar + bar_height;

    //Test if the mouse button is still pressed
    if (pressed) {
scroll_vertical:
      if (ui_active_object == self) {
        WORD scroll_value;

        switch (state) {
          case SBS_SCROLL:
            scroll_value = pointer_y - old_pointer_y;
            old_pointer_y = pointer_y;

            scroll_value = scroll_value * group->scroll.height / content_height;
            scrollGroupVert((struct UIO_Group*)self->parent, scroll_value);
          break;
          case SBS_PAGE_UP:
            if (page_delay < SB_PAGE_DELAY) {
              page_delay++;
              return;
            }
            page_delay = 0;
            scroll_value = -bar_height;

            scroll_value = scroll_value * group->scroll.height / content_height;
            scrollGroupVert((struct UIO_Group*)self->parent, scroll_value);
          break;
          case SBS_PAGE_DOWN:
            if (page_delay < SB_PAGE_DELAY) {
              page_delay++;
              return;
            }
            page_delay = 0;
            scroll_value = bar_height;

            scroll_value = scroll_value * group->scroll.height / content_height;
            scrollGroupVert((struct UIO_Group*)self->parent, scroll_value);
          break;
        }
      }
      else {
        //Check where has been clicked
        if (pointer_y < y1_bar) { // Top of the scrollbar
          ui_active_object = self;
          state = SBS_PAGE_UP;
          page_delay = SB_PAGE_DELAY;
          goto scroll_vertical;
        }
        else if (pointer_y > y2_bar) { // Bottom of the scrollbar
          ui_active_object = self;
          state = SBS_PAGE_DOWN;
          page_delay = SB_PAGE_DELAY;
          goto scroll_vertical;
        }
        else { // Scrollbar itself
          ui_active_object = self;
          state = SBS_SCROLL;
          old_pointer_y = pointer_y;
          self->flags |= UIOF_PRESSED;
          if (self->draw) self->draw(self);
        }
      }
    }
    else {
      if (self->flags & UIOF_PRESSED) {
        self->flags &= ~UIOF_PRESSED;
        if (self->draw) self->draw(self);
      }

      state = SBS_NONE;
      ui_active_object = NULL;
    }
  }
}
#endif // UI_USE_SCROLLBARS
///
///actionButton(self, pointer_x, pointer_y, pressed)
/******************************************************************************
 * Default onActive method for button objects. This function is called when   *
 * the button is clicked on and will be called each frame as long as the      *
 * mouse button is kept pressed. Makes the button clickable providing "relase *
 * verifiy".                                                                  *
 ******************************************************************************/
VOID actionButton(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD x2 = x + self->width;
  WORD y2 = y + self->height;

  //Test if the mouse button is still pressed
  if (pressed) {
    if (ui_active_object == self) {
      if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
        if (!(self->flags & UIOF_PRESSED)) {
          self->flags |= UIOF_PRESSED;
          if (self->draw) self->draw(self);
        }
      }
      else {
        if (self->flags & UIOF_PRESSED) {
          self->flags &= ~UIOF_PRESSED;
          if (self->draw) self->draw(self);
        }
      }
    }
    else {
      ui_active_object = self;
      self->flags |= UIOF_PRESSED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_PRESSED) {
      self->flags &= ~UIOF_PRESSED;
      if (self->draw) self->draw(self);
    }

    if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
      if (self->onAcknowledge) self->onAcknowledge(self);
    }

    ui_active_object = NULL;
  }
}
///
///actionCheckbox(self, pointer_x, pointer_y, pressed)
/******************************************************************************
 * Default onActive method for checkbox objects. This function is called when *
 * checkbox is clicked on.                                                    *
 ******************************************************************************/
#ifdef UI_USE_CHECKBOXES
VOID actionCheckbox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  if (pressed) {
    if (self != ui_active_object) {
      if (self->label) {
        struct UIO_Group* parent = (struct UIO_Group*)self->parent;
        WORD x = inheritX(self);
        WORD tick_box_width = self->height - 3;
        struct TextExtent te;

        TextExtent(ui_rastport, self->label, strlen(self->label), &te);

        if (pointer_x > x + tick_box_width + parent->child_gap + te.te_Width) {
          return;
        }
      }

      ui_active_object = self;
      self->flags ^= UIOF_PRESSED;
      if (self->draw) self->draw(self);
      if (self->onAcknowledge) self->onAcknowledge(self);
    }
  }
  else {
    if (self == ui_active_object) {
      ui_active_object = NULL;
    }
  }
}
#endif // UI_USE_CHECKBOXES
///
///actionCyclebox(self, pointer_x, pointer_y, pressed)
/******************************************************************************
 * Default onActive method for cyclebox objects. This function is called when *
 * the cyclebox is clicked on and will be called each frame as long as the    *
 * mouse button is kept pressed. Makes the button a "release verified"        *
 * clickable object.                                                          *
 ******************************************************************************/
#ifdef UI_USE_CYCLEBOXES
VOID actionCyclebox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD x2 = x + self->width;
  WORD y2 = y + self->height;

  //Test if the mouse button is still pressed
  if (pressed) {
    if (ui_active_object == self) {
      if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
        if (!(self->flags & UIOF_PRESSED)) {
          self->flags |= UIOF_PRESSED;
          if (self->draw) self->draw(self);
        }
      }
      else {
        if (self->flags & UIOF_PRESSED) {
          self->flags &= ~UIOF_PRESSED;
          if (self->draw) self->draw(self);
        }
      }
    }
    else {
      ui_active_object = self;
      self->flags |= UIOF_PRESSED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_PRESSED) {
      self->flags &= ~UIOF_PRESSED;
      if (self->draw) self->draw(self);
    }

    if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
      struct UIO_Cyclebox* cyclebox = (struct UIO_Cyclebox*)self;

      cyclebox->selected++;
      if (cyclebox->selected >= cyclebox->num_options) {
        cyclebox->selected = 0;
      }
      if (self->draw) self->draw(self);
      if (self->onAcknowledge) self->onAcknowledge(self);
    }

    ui_active_object = NULL;
  }
}
#endif // UI_USE_CYCLEBOXES
///
///actionString(self, pointer_x, pointer_y, pressed)
#ifdef UI_USE_INTEGER_GADGETS
/***********************************************************************
 * UTILITY FUNCTION: Takes a number in string form and converts the    *
 * value into a LONG int.                                              *
 * NOTE: This implementation only uses LONG integers while preventing  *
 * overflows. So values that exceed signed 32 bit integer limits       *
 * (UI_INT_MIN amd UI_INT_MAX) will be clipped to these values instead *
 * returning garbage overflowed values.                                *
 ***********************************************************************/
LONG aToi(STRPTR string, ULONG length)
{
  LONG value = 0;
  LONG test_overflow = 0;
  LONG digits = 10;
  ULONG negative = *string == '-';

  if (length) {
    STRPTR curs = string + length - 1; // Step on the last digit
    LONG multiplier = 1;

    for (; length > 0; length--) {
      if (*curs == '-') {
        curs--;
        continue;
      }
      if (digits < 1) break;
      if (digits == 1 && *curs > '2') {
        value = UI_INT_MAX;
        break;
      }
      test_overflow = value + ((*curs - '0') * multiplier);
      if (test_overflow < value) {
        value = UI_INT_MAX;
        break;
      }
      else value = test_overflow;
      multiplier *= 10;
      curs--;
      digits--;
    }
  }

  if (negative) value *= -1;

  return value;
}
#endif // UI_USE_INTEGER_GADGETS

/******************************************************************************
 * Default onActive method for string objects.                                *
 ******************************************************************************/
#ifdef UI_USE_STRING_GADGETS
VOID actionString(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed)
{
  struct UIO_String* string = (struct UIO_String*)self;
  WORD x = inheritX(self);
  WORD y = inheritY(self);
  WORD x2 = x + self->width;
  WORD y2 = y + self->height;
  struct TextExtent te;
  BOOL redraw_required = FALSE;
  #if UIOV_STRING_CURSOR_BLINK_DELAY > 0
  static ULONG blink_counter = 0;

  if (blink_counter > UIOV_STRING_CURSOR_BLINK_DELAY) {
    blink_counter = 0;
    self->flags ^= UIOF_CURSOR_ON;
    redraw_required = TRUE;
  }
  blink_counter++;
  #endif

  //Test if the mouse button is still pressed
  if (pressed) {
    if (self->flags & UIOF_PRESSED) {
      //Scroll contents
      if (pointer_x < x) {
        if (string->cursor_pos) string->cursor_pos--;
      }
      else if (pointer_x > x2) {
        if (string->cursor_pos < string->content_length) string->cursor_pos++;
      }
      else {
        string->cursor_pos = TextFit(ui_rastport, string->content, string->content_length, &te, NULL, 1, pointer_x - x + string->offset, self->height);
      }
      if (self->draw) self->draw(self);

      #if UIOV_STRING_CURSOR_BLINK_DELAY > 0
      blink_counter = 0;
      #endif
    }
    else {
      if (ui_active_object == self) {
        //Locate cursor on already active object
        if (IS_IN_RECT(pointer_x, pointer_y, x, y, x2, y2)) {
          string->cursor_pos = TextFit(ui_rastport, string->content, string->content_length, &te, NULL, 1, pointer_x - x + string->offset, self->height);
          self->flags |= UIOF_PRESSED;
          self->flags |= UIOF_CURSOR_ON;
        }
        else {
          //Deactivate object
          if (self->flags & UIOF_CURSOR_ON) {
            self->flags &= ~UIOF_CURSOR_ON;
          }
          ui_active_object = NULL;
          turnInputBufferOff();
          initInputBuffer();
        }

        if (self->draw) self->draw(self);

        #if UIOV_STRING_CURSOR_BLINK_DELAY > 0
        blink_counter = 0;
        #endif
      }
      else {
        //Activate object
        self->flags |= UIOF_PRESSED;
        ui_active_object = self;
        string->cursor_pos = TextFit(ui_rastport, string->content, string->content_length, &te, NULL, 1, pointer_x - x + string->offset, self->height);
        self->flags |= UIOF_CURSOR_ON;
        if (self->draw) self->draw(self);
        turnInputBufferOn();
      }
    }
  }
  else {
    UBYTE ch;

    self->flags &= ~UIOF_PRESSED;

    //Get input from system
    while ((ch = popInputBuffer())) {
      switch (ch) {
        case ASCII_HOME:
        string->cursor_pos = 0;
        break;
        case ASCII_END:
        string->cursor_pos = string->content_length;
        break;
        case ASCII_LEFT:
        if (string->cursor_pos > 0) string->cursor_pos--;
        break;
        case ASCII_RIGHT:
        if (string->cursor_pos < string->content_length) string->cursor_pos++;
        break;

        case ASCII_UP:
        break;
        case ASCII_DOWN:
        break;

        case ASCII_DEL:
        if (string->cursor_pos < string->content_length) {
          STRPTR cursor_addr = string->content + string->cursor_pos;
          memmove(cursor_addr, cursor_addr + 1, string->content_length - string->cursor_pos);
          string->content_length--;
        }
        break;
        case ASCII_DEL_ALL:
        if (string->cursor_pos < string->content_length) {
          STRPTR cursor_addr = string->content + string->cursor_pos;
          *cursor_addr = NULL;
          string->content_length = string->cursor_pos;
        }
        break;
        case ASCII_BACKSPACE:
        if (string->cursor_pos > 0) {
          STRPTR cursor_addr;
          string->cursor_pos--;
          cursor_addr = string->content + string->cursor_pos;
          memmove(cursor_addr, cursor_addr + 1, string->content_length - string->cursor_pos);
          string->content_length--;
        }
        break;
        case ASCII_BACKSPACE_ALL:
        if (string->cursor_pos > 0) {
          STRPTR cursor_addr;
          cursor_addr = string->content + string->cursor_pos;
          memmove(string->content, cursor_addr, string->content_length - string->cursor_pos + 1);
          string->content_length -= string->cursor_pos;
          string->cursor_pos = 0;
        }
        break;
        case ASCII_RETURN:
        case ASCII_TAB:
        case ASCII_BACKTAB:
        case ASCII_ESC:
          self->flags &= ~UIOF_CURSOR_ON;
          ui_active_object = NULL;
          turnInputBufferOff();
          initInputBuffer();
#ifdef UI_USE_INTEGER_GADGETS
          if (self->type == UIOT_INTEGER) {
            struct UIO_Integer* integer = (struct UIO_Integer*)self;
            setIntegerValue(integer, aToi(string->content, string->content_length), UI_REDRAW);
          }
          else
#endif // UI_USE_INTEGER_GADGETS
            if (self->draw) self->draw(self);

          if (string->onAcknowledge) string->onAcknowledge(self);
          return;
        break;
        default:
        if (string->content_length < string->max_length) {
          STRPTR cursor_addr;

          // Filter allowed or restricted characters
          if (string->allowed) {
            UBYTE* allowed_ch = string->allowed;
            ULONG found = FALSE;

            while (*allowed_ch) {
              if (ch == *allowed_ch) {
                found = TRUE;
                break;
              }
              allowed_ch++;
            }

            if (!found) return;
          }
          else if (string->restricted) {
            UBYTE* restricted_ch = string->restricted;
            while (*restricted_ch) {
              if (ch == *restricted_ch) return;
              restricted_ch++;
            }
          }

          // Insert new character into the string
          cursor_addr = string->content + string->cursor_pos;
          memmove(cursor_addr + 1, cursor_addr, string->content_length - string->cursor_pos + 1);
          *cursor_addr = ch;
          string->cursor_pos++;
          string->content_length++;
        }
        break;
      }
      redraw_required = TRUE;

      #if UIOV_STRING_CURSOR_BLINK_DELAY > 0
      blink_counter = 0;
      self->flags |= UIOF_CURSOR_ON;
      #endif
    }

    if (redraw_required) {
      if (self->draw) self->draw(self);
    }
  }
}
#endif // UI_USE_STRING_GADGETS
///

/*************************
 * HOVER FUNCTIONS       *
 *************************/
///hoverTabSelector(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for tab selector objects.                           *
 ******************************************************************************/
#ifdef UI_USE_TABBED_GROUPS
VOID hoverTabSelector(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE

}
#endif // UI_USE_TABBED_GROUPS
///
///hoverGroup(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for group objects.                                  *
 ******************************************************************************/
VOID hoverGroup(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE

}
///
///hoverScrollbar(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for scrollbar objects.                              *
 ******************************************************************************/
#ifdef UI_USE_SCROLLBARS
VOID hoverScrollbar(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE
  if (hovered) {
    if (self->flags & UIOF_HOVERED) {
      // We can set a counter here to display bubble help or animate while being
      // hovered.
    }
    else {
      self->flags |= UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_HOVERED) {
      self->flags &= ~UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
}
#endif // UI_USE_SCROLLBARS
///
///hoverLabel(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for label objects.                                  *
 ******************************************************************************/
#ifdef UI_USE_LABELS
VOID hoverLabel(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE

}
#endif // UI_USE_LABELS
///
///hoverButton(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for button objects.                                 *
 ******************************************************************************/
VOID hoverButton(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE
  if (hovered) {
    if (self->flags & UIOF_HOVERED) {
      // We can set a counter here to display bubble help or animate while being
      // hovered.
    }
    else {
      self->flags |= UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_HOVERED) {
      self->flags &= ~UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
}
///
///hoverCheckbox(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for checkbox objects.                               *
 ******************************************************************************/
#ifdef UI_USE_CHECKBOXES
VOID hoverCheckbox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE
  if (hovered) {
    if (self->flags & UIOF_HOVERED) {
      // We can set a counter here to display bubble help or animate while being
      // hovered.
    }
    else {
      self->flags |= UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_HOVERED) {
      self->flags &= ~UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
}
#endif // UI_USE_CHECKBOXES
///
///hoverCyclebox(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for cyclebox objects.                               *
 ******************************************************************************/
#ifdef UI_USE_CYCLEBOXES
VOID hoverCyclebox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE
  if (hovered) {
    if (self->flags & UIOF_HOVERED) {
      // We can set a counter here to display bubble help or animate while being
      // hovered.
    }
    else {
      self->flags |= UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_HOVERED) {
      self->flags &= ~UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
}
#endif // UI_USE_CYCLEBOXES
///
///hoverString(self, pointer_x, pointer_y, hovered)
/******************************************************************************
 * Default onHover method for string objects.                                 *
 ******************************************************************************/
#ifdef UI_USE_STRING_GADGETS
VOID hoverString(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered)
{
  // IMPLEMENT YOUR ON HOVER ACTIVITIES HERE
  if (hovered) {
    if (self->flags & UIOF_HOVERED) {
      // We can set a counter here to display bubble help or animate while being
      // hovered.
    }
    else {
      self->flags |= UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
  else {
    if (self->flags & UIOF_HOVERED) {
      self->flags &= ~UIOF_HOVERED;
      if (self->draw) self->draw(self);
    }
  }
}
#endif // UI_USE_STRING_GADGETS
///

/*************************
 * ACCESS FUNCTIONS      *
 *************************/
///setStringContents(object, string, redraw)
/******************************************************************************
 * Copies given string into string object's private buffer respecting its     *
 * buffer size and redraws the string object.                                 *
 * WARNING: Never redraw when the ui is not initialized to a valid (currently *
 * on display) rastport!                                                      *
 ******************************************************************************/
#ifdef UI_USE_STRING_GADGETS
VOID setStringContents(struct UIO_String* object, STRPTR string, BOOL redraw)
{
  ULONG string_len = strlen(string);
  ULONG copy_length = MIN(string_len, object->max_length);
  strncpy(object->content, string, copy_length);
  if (redraw && object->draw) object->draw((struct UIObject*)object);
}
#endif // UI_USE_STRING_GADGETS
///
///setIntegerValue(object, value, redraw)
/******************************************************************************
 * Sets the given LONG value into an integer gadget respecting min/max        *
 * restrictions and doing the necessary value/string conversions.             *
 ******************************************************************************/
#ifdef UI_USE_INTEGER_GADGETS
VOID setIntegerValue(struct UIO_Integer* object, LONG value, BOOL redraw)
{
  if (value > object->max) value = object->max;
  if (value < object->min) value = object->min;
  object->value = value;
  sprintf(object->string.content, "%ld", value);
  object->string.content_length = strlen(object->string.content);
  if (redraw && object->string.draw) object->string.draw((struct UIObject*)object);
}
#endif // UI_USE_INTEGER_GADGETS
///
///incrementIntegerValue(object)
/******************************************************************************
 * Increments the value on an integer gadget by the amount set in the member  *
 * increment respecting max restriction and doing the necessary value/string  *
 * conversions.                                                               *
 ******************************************************************************/
#ifdef UI_USE_INTEGER_GADGETS
VOID incrementIntegerValue(struct UIO_Integer* integer, BOOL redraw)
{
  setIntegerValue(integer, integer->value + integer->increment, redraw);
}
#endif // UI_USE_INTEGER_GADGETS
///
///decrementIntegerValue(object)
/******************************************************************************
 * Decrements the value on an integer gadget by the amount set in the member  *
 * increment respecting min restriction and doing the necessary value/string  *
 * conversions.                                                               *
 ******************************************************************************/
#ifdef UI_USE_INTEGER_GADGETS
VOID decrementIntegerValue(struct UIO_Integer* integer, BOOL redraw)
{
  setIntegerValue(integer, integer->value - integer->increment, redraw);
}
#endif // UI_USE_INTEGER_GADGETS
///

/*************************
 * ACKNOWLEDGE FUNCTIONS *
 *************************/
///onClickIncrementInteger(self)
#ifdef UI_USE_INTEGER_GADGETS
VOID onClickIncrementInteger(struct UIObject* self)
{
  struct UIO_Group* parent = (struct UIO_Group*)self->parent;
  struct UIO_Integer* integer = (struct UIO_Integer*)parent->children[0];
  incrementIntegerValue(integer, UI_REDRAW);
}
#endif // UI_USE_INTEGER_GADGETS
///
///onClickDecrementInteger(self)
#ifdef UI_USE_INTEGER_GADGETS
VOID onClickDecrementInteger(struct UIObject* self)
{
  struct UIO_Group* parent = (struct UIO_Group*)self->parent;
  struct UIO_Integer* integer = (struct UIO_Integer*)parent->children[0];
  decrementIntegerValue(integer, UI_REDRAW);
}
#endif // UI_USE_INTEGER_GADGETS
///
