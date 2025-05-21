# Generic Amiga cross-compiler makefile
#  - for m68k OS3 binaries use: make
#  - for ppc OS4 binaries use: make OS=os4
#  - for ppc MorphOS binaries use: make OS=mos

EXE = BOBSheeter
################################################################################
# Target OS
ifndef (OS)
  OS = os3
endif

ifeq ($(OS), os3)
  CPU = -m68020
  CC = m68k-amigaos-gcc
  OPTIONS = -DNO_INLINE_STDARG
  LFLAGS = -s -noixemul -lamiga
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
IDIRS = 

CFLAGS = $(WARNINGS) $(OPTIMIZE) $(DEBUG) $(CPU) $(OPTIONS) $(IDIRS)

OBJS = main.o
################################################################################

# target 'all' (default target)
all : $(EXE)

# $@ matches the target; $< matches the first dependent
main.o : main.c
	$(CC) -c $< $(CFLAGS)

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LFLAGS)

# target 'clean'
clean:
	rm -f $(EXE)
	rm -f $(OBJS)
