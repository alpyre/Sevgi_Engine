# Generic Amiga cross-compiler makefile
#  - for m68k OS3 binaries use: make
#  - for ppc OS4 binaries use: make OS=os4
#  - for ppc MorphOS binaries use: make OS=mos

EXE = Sevgi_Editor
################################################################################
# Target OS
ifndef (OS)
  OS = os3
endif

ifeq ($(OS), os3)
  CPU = -m68020
  CC = m68k-amigaos-gcc
  OPTIONS = -DNO_INLINE_STDARG
  LFLAGS = -s -noixemul -lamiga -lmui
else
ifeq ($(OS), os4)
  CPU = -mcpu=powerpc
  CC = ppc-amigaos-gcc
  OPTIONS = -DNO_INLINE_STDARG -D__USE_INLINE__
  LFLAGS = -lauto
else
ifeq ($(OS), mos)
  CPU = -mcpu=powerpc
  CC = ppc-morphos-gcc
  OPTIONS = -DNO_PPCINLINE_STDARG
  LFLAGS = -s -noixemul
endif
endif
endif
################################################################################
# Common options
WARNINGS = -Wall
OPTIMIZE = -Os
DEBUG =
IDIRS = -Iincludes

CFLAGS = $(WARNINGS) $(OPTIMIZE) $(DEBUG) $(CPU) $(OPTIONS) $(IDIRS)

OBJS = main.o dosupernew.o utility.o toolbar.o tooltypes.o										 \
			 popasl_string.o ackstring.o integer_gadget.o gamefont_creator.o				 \
			 tileset_creator.o tilemap_creator.o bobsheet_creator.o									 \
			 spritebank_creator.o	color_gadget.o color_palette.o palette_editor.o		 \
			 game_settings.o assets_editor.o								                         \
			 ILBM_image.o chunky_image.o image_spec.o image_display.o image_editor.o \
			 dtpicdisplay.o new_project.o editor_settings.o palette_selector.o       \
			 display_creator.o bitmap_selector.o
################################################################################

# target 'all' (default target)
all : $(EXE)

# $@ matches the target; $< matches the first dependent
main.o : main.c
	$(CC) -c $< $(CFLAGS)

bitmap_selector.o : bitmap_selector.c
	$(CC) -c $< $(CFLAGS)

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LFLAGS)

# target 'clean'
clean:
	rm -f $(EXE)
	rm -f $(OBJS)
	find . -name '*.uaem' -delete
