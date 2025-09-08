#ifndef	UI_H
#define	UI_H

/******************************************************************************
 * UI SETTINGS                                                                *
 * NOTE: Enable/disable code sections for UI object types that you plan to    *
 * use or not in your game UI.                                                *
 ******************************************************************************/
#define UI_USE_TABBED_GROUPS
#define UI_USE_SCROLLING
#define UI_USE_SCROLLBARS
#define UI_USE_RECTANGLES
#define UI_USE_SEPARATORS
#define UI_USE_LABELS
#define UI_USE_CHECKBOXES
#define UI_USE_CYCLEBOXES
#define UI_USE_STRING_GADGETS
#define UI_USE_INTEGER_GADGETS
#define UI_USE_SLIDERS
//#define UI_USE_LISTVIEWS
//#define UI_USE_RELAYOUTING
//#define UI_SCROLL_USING_REDRAW

/******************************************************************************
 * UI PENS                                                                    *
 * NOTE: Match these with the colors on your display's palette if you will    *
 * use the draw functions from the API in your render functions.              *
 ******************************************************************************/
#define UIPEN_BACKGROUND   0
#define UIPEN_FILL         0
#define UIPEN_SHADOW       1
#define UIPEN_SHINE        3
#define UIPEN_TEXT         3
#define UIPEN_DETAIL       2
#define UIPEN_HIGHLIGHT    2
#define UIPEN_HALF_SHINE   1
#define UIPEN_HALF_SHADOW  2

/******************************************************************************
 * UI SIZE and COLORS                                                         *
 * NOTE: If you decide to rewrite the renderers (like drawFrame() function)   *
 * you should edit the sizes defined below accordingly for proper UI layout.  *
 ******************************************************************************/
#define UIOV_GROUP_MIN_WIDTH    8
#define UIOV_GROUP_MIN_HEIGHT   8
#define UIOV_GROUP_FRAME_TEXT   6
#define UIOV_GROUP_FRAME_WIDTH  2
#define UIOV_GROUP_FRAME_HEIGHT 2
#define UIOV_GROUP_FRAME_MARGIN 1

#define UIOV_SCROLLBAR_HEIGHT 6
#define UIOV_SCROLLBAR_WIDTH  6
#define UIOV_SCROLLBAR_BAR_COLOR    UIPEN_HALF_SHADOW
#define UIOV_SCROLLBAR_PAGE_COLOR   UIPEN_HALF_SHINE
#define UIOV_SCROLLBAR_ACTIVE_COLOR UIPEN_SHINE

#define UIOV_TAB_FRAME_WIDTH   1
#define UIOV_TAB_FRAME_HEIGHT  1
#define UIOV_TAB_MARGIN_TOP    3
#define UIOV_TAB_MARGIN_BOTTOM 1
#define UIOV_TAB_MARGIN_SIDE   1
#define UIOV_TAB_SELECTED_RISE 1
#define UIOV_TAB_NOTCH         1

#define UIOV_BUTTON_FRAME_WIDTH   1
#define UIOV_BUTTON_FRAME_HEIGHT  1
#define UIOV_BUTTON_MARGIN_TOP    3
#define UIOV_BUTTON_MARGIN_BOTTOM 1
#define UIOV_BUTTON_MARGIN_SIDE   1

#define UIOV_STRING_FRAME_WIDTH        2
#define UIOV_STRING_FRAME_HEIGHT       2
#define UIOV_STRING_MARGIN_TOP         1
#define UIOV_STRING_MARGIN_BOTTOM      1
#define UIOV_STRING_MARGIN_SIDE        1
#define UIOV_STRING_PLACEHOLDER_PEN    UIPEN_HALF_SHADOW
#define UIOV_STRING_CURSOR_PEN         UIPEN_DETAIL
#define UIOV_STRING_CURSOR_WIDTH_CHAR  0xFFFF
#define UIOV_STRING_CURSOR_WIDTH       3
#define UIOV_STRING_CURSOR_BLINK_DELAY 25
#define UIOV_STRING_PASSWORD_CHAR      '*'
#define UIOV_STRING_DOTTED_CHAR        '.'

#define UIOV_CYCLEBOX_SEPARATOR_WIDTH  4

#define UIOV_SLIDER_TRACK_HEIGHT          2
#define UIOV_SLIDER_TRACK_WIDTH           2
#define UIOV_SLIDER_HANDLE_HEIGHT         8
#define UIOV_SLIDER_HANDLE_WIDTH          8
#define UIOV_SLIDER_HANDLE_SIZE           2
#define UIOV_SLIDER_HANDLE_COLOR          UIPEN_HALF_SHINE
#define UIOV_SLIDER_HANDLE_SELECTED_COLOR UIPEN_SHINE
#define UIOV_SLIDER_STOP_COLOR            UIPEN_HALF_SHADOW
#define UIOV_SLIDER_VALUE_COLOR           UIPEN_SHINE
#define UIOV_SLIDER_VALUE_SPACING         1
#define UIOV_SLIDER_ELAPSED_COLOR         UIPEN_DETAIL

/******************************************************************************
 *                       END OF CUSTOMIZABLE SECTION                          *
 ******************************************************************************/

 #define MAX(a, b) ((a) >= (b) ? (a) : (b))
 #define MIN(a, b) ((a) <= (b) ? (a) : (b))

 //Centers width a in width b by calculating the start position of a
 //|B------|A-------|-------|
 #define CENTER(a, b) (((b) - (a)) / 2)

/******************************************************************************
 * CONTROL VALUES                                                             *
 ******************************************************************************/
#define UI_REDRAW           TRUE
#define UI_NO_REDRAW        FALSE
#define UI_CALCULATE_WIDTH  1
#define UI_CALCULATE_HEIGHT 2
#define UI_INT_MIN          0x80000000
#define UI_INT_MAX          0x7FFFFFFF

/******************************************************************************
 * UI OBJECT TYPES                                                            *
 ******************************************************************************/
#define UIOT_GROUP            1
#define UIOT_SCROLLBAR        2
#define UIOT_TABSELECTOR      3
#define UIOT_RECTANGLE        4
#define UIOT_SEPARATOR        5
#define UIOT_LABEL            6
#define UIOT_BUTTON           7
#define UIOT_CHECKBOX         8
#define UIOT_CYCLEBOX         9
#define UIOT_STRING          10
#define UIOT_INTEGER         11
#define UIOT_SLIDER          12

/******************************************************************************
 * UI OBJECT FLAGS                                                            *
 ******************************************************************************/
#define UIOF_NONE             0x000
#define UIOF_DISABLED         0x001
#define UIOF_SELECTED         0x002
#define UIOF_HOVERED          0x004
#define UIOF_PRESSED          0x008
#define UIOF_INHERIT_X        0x010
#define UIOF_INHERIT_Y        0x020
#define UIOF_INHERIT_WIDTH    0x040
#define UIOF_INHERIT_HEIGHT   0x080
#define UIOF_INHERIT_POS      (UIOF_INHERIT_X | UIOF_INHERIT_Y)
#define UIOF_INHERIT_SIZE     (UIOF_INHERIT_WIDTH | UIOF_INHERIT_HEIGHT)
#define UIOF_INHERIT_ALL      (UIOF_INHERIT_POS | UIOF_INHERIT_SIZE)
#define UIOF_HORIZONTAL       0x100
#define UIOF_VERTICAL         UIOF_NONE
#define UIOF_FRAMED           0x200
#define UIOF_TITLED           0x400
#define UIOF_TABBED           0x800
// UIOT_STRING type specific flags
// *******************************
#define UIOF_CURSOR_ON        0x100
#define UIOF_PASSWORD         0x400
#define UIOF_DOTTED           0x800
// UIOT_CHECKBOX type specific flags
// *********************************
#define UIOF_CHECKED          0x008
// UIOT_LABEL type specific flags
// *********************************
#define UIOF_ALIGN_LEFT       UIOF_NONE
#define UIOF_ALIGN_CENTER     0x001
#define UIOF_ALIGN_RIGHT      0x002
#define UIOF_ALIGN_TOP        UIOF_NONE
#define UIOF_ALIGN_CENTER_V   0x400
#define UIOF_ALIGN_BOTTOM     0x800
// UIOT_SLIDER type specific flags
// *********************************
#define UIOF_DRAW_VALUE       0x400
#define SLIDER_STRICT         0x1
#define SLIDER_ELAPSED        0x2

// drawFrame() style flags
// NOTE: compatible with UIOF_ flags
#define FS_DISABLED 0x0001
#define FS_SELECTED 0x0002
#define FS_HOVERED  0x0004
#define FS_PRESSED  0x0008
#define FS_BUTTON   0x0000
#define FS_STRING   0x1000
#define FS_GROUP    FS_STRING | FS_PRESSED

/******************************************************************************
 * UI OBJECT MACROS                                                           *
 ******************************************************************************/
#define UI_ROOT(name, children) UI_GROUP(name, "Root", UIOF_INHERIT_ALL, children, 1, 1, 0)
#define UI_GROUP(name, label, flags, children, child_gap, columns, margin) UI_SIZED_GROUP(name, label, 0,0,0,0, flags, children, child_gap, columns, margin, NULL, NULL)
#define UI_SIZED_GROUP(name, label, x, y, width, height, flags, children, child_gap, columns, margin, h_scrbar, v_scrbar) STATIC struct UIO_Group name = {{NULL,NULL}, 0, 0, UIOT_GROUP, flags, label, NULL, x,y,width,height, sizeGroup, drawGroup, hoverGroup, NULL, NULL, UI_ANIM_STRUCT, children, child_gap, columns, {margin, margin, margin, margin}, {0,0,0,0, h_scrbar, v_scrbar}}

#define UI_SCROLLBAR(name, orientation) STATIC UI_OBJECT(name, 0, UIOT_SCROLLBAR, orientation, #name, 0,0,0,0, NULL, drawScrollbar, hoverScrollbar, actionScrollbar, NULL, 0)

#define UI_SCROLL_GROUP(name, label, x, y, width, height, flags, children, child_gap, columns, margin)\
UI_SCROLLBAR(name ## _scrollbar_horizontal, UIOF_HORIZONTAL);\
UI_SCROLLBAR(name ## _scrollbar_vertical, UIOF_VERTICAL);\
UI_SIZED_GROUP(name, label, x, y, width, height, flags, children, child_gap, columns, margin, &name ## _scrollbar_horizontal, &name ## _scrollbar_vertical)

#define UI_SCROLL_GROUP_VERT(name, label, x, y, width, height, flags, children, child_gap, columns, margin)\
UI_SCROLLBAR(name ## _scrollbar_vertical, UIOF_VERTICAL);\
UI_SIZED_GROUP(name, label, x, y, width, height, flags, children, child_gap, columns, margin, NULL, &name ## _scrollbar_vertical)

#define UI_SCROLL_GROUP_HORIZ(name, label, x, y, width, height, flags, children, child_gap, columns, margin)\
UI_SCROLLBAR(name ## _scrollbar_horizontal, UIOF_HORIZONTAL);\
UI_SIZED_GROUP(name, label, x, y, width, height, flags, children, child_gap, columns, margin, &name ## _scrollbar_horizontal, NULL)

#define UI_TAB(name, label, flags, children, margin)\
STATIC UBYTE name ## _tab_widths[sizeof(children) / sizeof(children[0]) - 1] = {0};\
STATIC struct UIObject name ## _tab_selector = {{NULL,NULL}, 0, 0, UIOT_TABSELECTOR, flags, label "_tab_selector", NULL, 0,0,0,0, sizeTabSelector, drawTabSelector, hoverTabSelector, actionTabSelector, NULL, UI_ANIM_STRUCT};\
STATIC struct UIO_Group name = {{NULL,NULL}, 0, 0, UIOT_GROUP, flags | UIOF_TABBED, label, NULL, 0,0,0,0, sizeGroup, drawGroup, hoverGroup, NULL, NULL, UI_ANIM_STRUCT, children, 0, 1, {margin, margin, margin, margin}, {0,0,0,sizeof(children) / sizeof(children[0]) - 1, (struct UIObject*) name ## _tab_widths, &name ## _tab_selector}}

#define UI_OBJECT(name, id, type, flags, label, x, y, width, height, size, draw, hover, action, acknowledge, initialized) struct UIObject name = {{NULL,NULL}, id, initialized, type, flags, label, NULL, x, y, width, height, size, draw, hover, action, acknowledge, UI_ANIM_STRUCT}
#define UI_RECTANGLE(name) STATIC UI_OBJECT(name, 0, UIOT_RECTANGLE, UIOF_INHERIT_POS, NULL, 0,0,0,0, sizeRectangle, NULL, NULL, NULL, NULL, 0)
#define UI_SEPARATOR(name, flags) STATIC UI_OBJECT(name, 0, UIOT_SEPARATOR, UIOF_INHERIT_POS | (flags), NULL, 0,0,0,0, sizeSeparator, drawSeparator, NULL, NULL, NULL, 0)
#define UI_LABEL(name, label, flags, y_offset) UI_OBJECT(name, 0, UIOT_LABEL, flags, label, 0,0,0,0, sizeLabel, drawLabel, hoverLabel, NULL, NULL, y_offset)
#define UI_BUTTON(name, label, flags, onClick) UI_OBJECT(name, 0, UIOT_BUTTON, flags, label, 0,0,0,0, sizeButton, drawButton, hoverButton, actionButton, onClick, 0)
#define UI_CHECKBOX(name, label, flags, onClick) UI_OBJECT(name, 0, UIOT_CHECKBOX, flags, label, 0,0,0,0, sizeCheckbox, drawCheckbox, hoverCheckbox, actionCheckbox, onClick, 0)

#define UI_CYCLEBOX(name, options, num_options, initial_selected, flags, onClick) struct UIO_Cyclebox name = {{NULL,NULL}, 0, 0, UIOT_CYCLEBOX, flags, #name, NULL, 0,0,0,0, sizeCyclebox, drawCyclebox, hoverCyclebox, actionCyclebox, onClick, UI_ANIM_STRUCT, options, num_options, initial_selected}

#define UI_STRING(name, flags, max_length, initial_content, placeholder, onAcknowledge)\
UBYTE name ## _string_content[max_length + 1] = {0};\
struct UIO_String name = {{NULL,NULL}, 0, 0, UIOT_STRING, flags, #name, NULL, 0,0,0,0, sizeString, drawString, hoverString, actionString, onAcknowledge, UI_ANIM_STRUCT, name ## _string_content, initial_content, placeholder, NULL, NULL, 0, max_length, 0, 0}

#define UI_INTEGER(name, flags, initial_value, min, max, increment, onAcknowledge)\
UBYTE name ## _integer_content[16] = {0};\
struct UIO_Integer name = {{{NULL,NULL}, 0, 0, UIOT_INTEGER, flags, #name, NULL, 0,0,0,0, sizeString, drawString, hoverString, actionString, onAcknowledge, UI_ANIM_STRUCT, name ## _integer_content, #initial_value, NULL, "+-0123456789", NULL, 0, 11, 0, 0}, initial_value, min, max, increment}

#define UI_INCDECINT(name, flags, initial_value, min, max, increment, onAcknowledge, child_gap)\
UI_INTEGER(name, flags, initial_value, min, max, increment, onAcknowledge);\
STATIC UI_BUTTON(name ## _button_plus, "+", ((flags) & UIOF_FRAMED) | UIOF_INHERIT_X | UIOF_INHERIT_Y | UIOF_INHERIT_HEIGHT, onClickIncrementInteger);\
STATIC UI_BUTTON(name ## _button_minus, "-", ((flags) & UIOF_FRAMED) | UIOF_INHERIT_X | UIOF_INHERIT_Y | UIOF_INHERIT_HEIGHT, onClickDecrementInteger);\
STATIC struct UIObject* name ## _group_children[] = {(struct UIObject*)&name, (struct UIObject*)&name ## _button_plus, (struct UIObject*)&name ## _button_minus, NULL};\
UI_GROUP(name ## _group, #name "_group", UIOF_HORIZONTAL | ((flags) & UIOF_INHERIT_ALL), name ## _group_children, child_gap, 1, 0)

#define UI_SLIDER(name, flags, slider_flags, initial_value, min, max, increment, mark_increment, onUpdate)\
struct UIO_Slider name = {{NULL,NULL}, 0, slider_flags, UIOT_SLIDER, flags, NULL, NULL, 0, 0, 0, 0, sizeSlider, drawSlider, hoverSlider, actionSlider, onUpdate, UI_ANIM_STRUCT, initial_value, min, max, increment, mark_increment}

/******************************************************************************
 * UI STORAGE TYPES                                                           *
 ******************************************************************************/
typedef UBYTE STRLEN;

// UIObject union members
#define u_tab_selected scroll.width
#define u_tab_numtabs scroll.height
#define u_tab_widths scroll.bar_horiz
#define u_tab_selector scroll.bar_vert
#define u_label_y_offset initialized
#define u_slider_flags initialized

/******************************************************************************
 * UI OBJECT STRUCTS                                                          *
 ******************************************************************************/
struct UIObject; // Forward declare because this will be self referenced

#define UI_ANIM_STRUCT {0,0,0,0,0,0,0,0,NULL}
struct UIO_Anim {
  UBYTE type;
  UBYTE state;
  UBYTE frame;
  UBYTE direction;
  UBYTE speed;
  UBYTE counter_1;
  UBYTE counter_2;
  UBYTE counter_3;
  VOID (*func)(struct UIObject*, WORD, WORD, BOOL);
};

// COMMON UI OBJECT MEMBERS
// ************************
#define UIOBJECT_COMMON \
  struct MinNode node;\
  UWORD id;\
  BYTE  initialized;\
  UBYTE type;\
  ULONG flags;\
  STRPTR label;\
  struct UIObject* parent;\
  WORD x;\
  WORD y;\
  UWORD width;\
  UWORD height;\
  VOID (*calcSize)(struct UIObject*);\
  VOID (*draw)(struct UIObject*);\
  VOID (*onHover)(struct UIObject*, WORD, WORD, BOOL);\
  VOID (*onActive)(struct UIObject*, WORD, WORD, BOOL);\
  VOID (*onAcknowledge)(struct UIObject*);\
  struct UIO_Anim anim
  // TYPE SPECIFIC MEMBERS FOLLOW


// UI OBJECT (access to common members)
// ************************************
struct UIObject {
  UIOBJECT_COMMON;
  // TYPE SPECIFIC MEMBERS FOLLOW
};


// LAYOUT GROUP OBJECT
// *******************
struct UIO_Group {
  UIOBJECT_COMMON;
  struct UIObject** children;
  UWORD child_gap;
  UWORD columns;
  struct {
    UBYTE top;
    UBYTE bottom;
    UBYTE left;
    UBYTE right;
  }margin;
  struct {
    WORD x;
    WORD y;
    UWORD width;
    UWORD height;
    struct UIObject* bar_horiz;
    struct UIObject* bar_vert;
  }scroll;
};

// STRING INPUT OBJECT
// *******************
struct UIO_String {
  UIOBJECT_COMMON;
  STRPTR content;
  STRPTR initial;
  STRPTR placeholder;
  STRPTR allowed;
  STRPTR restricted;
  STRLEN content_length;
  STRLEN max_length;
  STRLEN cursor_pos;
  WORD offset;
};

// INTEGER INPUT OBJECT
// ********************
struct UIO_Integer {
  struct UIO_String string;
  LONG value;
  LONG min;
  LONG max;
  LONG increment;
};

// CYCLEBOX OBJECT
// ***************
struct UIO_Cyclebox {
  UIOBJECT_COMMON;
  STRPTR* options;
  UWORD num_options;
  UWORD selected; //The index of the currently selected option string in options
};

// SLIDER OBJECT
// *************
struct UIO_Slider {
  UIOBJECT_COMMON;
  LONG value;
  LONG min;
  LONG max;
  UWORD increment;
  UWORD mark_increment;
};

/******************************************************************************
 * UI FUNCTION PROTOS                                                         *
 ******************************************************************************/
VOID initUI(ULONG screen_start, struct RastPort* rastport, struct UIO_Group* root, struct UIObject** cycle_chain);
VOID resetUI(VOID);
VOID resetUILayout(struct UIO_Group* root);
VOID updateUI(struct UIO_Group* root, WORD pointer_x, WORD pointer_y, BOOL pressed);
WORD inheritX(struct UIObject* obj);
WORD inheritY(struct UIObject* obj);
VOID setClip(struct UIObject* obj);
VOID removeClip(VOID);
ULONG selectObject(struct UIObject* object);
struct UIObject* selectObjectAtIndex(ULONG index);
ULONG setIndexToObject(struct UIObject* object);
ULONG getSelectedIndex(VOID);
struct UIObject* getSelectedObject(VOID);
struct UIObject* nextObject(VOID);
struct UIObject* prevObject(VOID);
struct UIObject* activateObject(VOID);

/******************************************************************************
 * UI OBJECT FUNCTIONS                                                        *
 ******************************************************************************/
// GROUP OBJECT
// ************
VOID sizeGroup(struct UIObject* self);
VOID drawGroup(struct UIObject* self);
VOID hoverGroup(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
// scroll groups
VOID drawScrollbar(struct UIObject* self);
VOID hoverScrollbar(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionScrollbar(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);
// tabbed groups
VOID sizeTabSelector(struct UIObject* self);
VOID drawTabSelector(struct UIObject* self);
VOID hoverTabSelector(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionTabSelector(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);

// RECTANGLE OBJECT
// ****************
VOID sizeRectangle(struct UIObject* self);

// SEPARATOR OBJECT
// ****************
VOID sizeSeparator(struct UIObject* self);
VOID drawSeparator(struct UIObject* self);

// LABEL OBJECT
// ************
VOID sizeLabel(struct UIObject* self);
VOID drawLabel(struct UIObject* self);
VOID hoverLabel(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);

// BUTTON OBJECT
// *************
VOID sizeButton(struct UIObject* self);
VOID drawButton(struct UIObject* self);
VOID hoverButton(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionButton(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);

// CHECKBOX OBJECT
// ***************
VOID sizeCheckbox(struct UIObject* self);
VOID drawCheckbox(struct UIObject* self);
VOID hoverCheckbox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionCheckbox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);
#define isChecked(checkbox) (checkbox->flags & UIOF_CHECKED)

// CYCLEBOX OBJECT
// ***************
VOID sizeCyclebox(struct UIObject* self);
VOID drawCyclebox(struct UIObject* self);
VOID hoverCyclebox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionCyclebox(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);

// STRING OBJECT
// *************
VOID sizeString(struct UIObject* self);
VOID drawString(struct UIObject* self);
VOID hoverString(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionString(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);
VOID setStringContents(struct UIO_String* object, STRPTR string, BOOL redraw);

// INTEGER OBJECT
// **************
// NOTE: Uses the above string object functions for size, draw, hover and action
VOID setIntegerValue(struct UIO_Integer* object, LONG value, BOOL redraw);
VOID incrementIntegerValue(struct UIO_Integer* integer, BOOL redraw);
VOID decrementIntegerValue(struct UIO_Integer* integer, BOOL redraw);
VOID onClickIncrementInteger(struct UIObject* self);
VOID onClickDecrementInteger(struct UIObject* self);

// SLIDER OBJECT
// *************
VOID sizeSlider(struct UIObject* self);
VOID drawSlider(struct UIObject* self);
VOID hoverSlider(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL hovered);
VOID actionSlider(struct UIObject* self, WORD pointer_x, WORD pointer_y, BOOL pressed);
VOID setSliderValue(struct UIO_Slider* slider, LONG value, BOOL redraw);

#endif /* UI_H */
