#ifndef TOOLBAR_H
#define TOOLBAR_H

#define TOOLBAR_IMAGE_DIR "Images"
#define TOOLBAR_IMAGE_STYLE "MWB_20x20"
#define TOOLBAR_BUTTON_WIDTH  20
#define TOOLBAR_BUTTON_HEIGHT 20

#define TOOLBAR_IMAGE_FILES "new.iff",     \
                            "load.iff",    \
                            "save.iff",    \
                            "saveas.iff",  \
                            "undo.iff",    \
                            "redo.iff",    \
                            "cut.iff",     \
                            "copy.iff",    \
                            "paste.iff",   \
                            "font.iff",    \
                            "tileset.iff", \
                            "map.iff",     \
                            "boing.iff",   \
                            "sprite.iff",  \
                            "image.iff",   \
                            "objects.iff", \
                            "palette.iff", \
                            "gear.iff",    \
                            "linker.iff",  \
                            "editor.iff",  \
                            "new_display.iff"

enum {
  IMG_SPEC_NEW,
  IMG_SPEC_LOAD,
  IMG_SPEC_SAVE,
  IMG_SPEC_SAVEAS,
  IMG_SPEC_UNDO,
  IMG_SPEC_REDO,
  IMG_SPEC_CUT,
  IMG_SPEC_COPY,
  IMG_SPEC_PASTE,
  IMG_SPEC_FONT,
  IMG_SPEC_TILE,
  IMG_SPEC_MAP,
  IMG_SPEC_BOB,
  IMG_SPEC_SPRITE,
  IMG_SPEC_IMAGE,
  IMG_SPEC_OBJECTS,
  IMG_SPEC_PALETTE,
  IMG_SPEC_SETTINGS,
  IMG_SPEC_ASSETS,
  IMG_SPEC_EDITOR,
  IMG_SPEC_DISPLAY
};

#define MUIA_Enabled 0 //Not a real attribute, just a value to make reading easier

//Prototypes
BOOL createImageSpecs();
VOID freeImageSpecs();
Object* MUI_NewImageButton(ULONG spec, STRPTR short_help, ULONG disabled);
Object* MUI_NewImageButtonSeparator();
Object* MUI_HorizontalSeparator();

#endif /* TOOLBAR_H */
