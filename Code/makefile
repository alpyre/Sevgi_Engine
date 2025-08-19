# Makefile for Sevgi_Engine projects (gcc)

EXE = sevgi_engine
################################################################################
CPU = -m68020
AS = vasm
CC = m68k-amigaos-gcc -c
LD = m68k-amigaos-gcc -o
OPTIONS = -DNO_INLINE_STDARG
LFLAGS = -s -noixemul -lamiga
################################################################################
# Common options
WARNINGS = -Wall
OPTIMIZE = -Os
DEBUG =
IDIRS =

CFLAGS = $(WARNINGS) $(OPTIMIZE) $(DEBUG) $(CPU) $(OPTIONS) $(IDIRS)
AFLAGS = -Faout $(CPU)

DEPS = assets.h palettes.h settings.h version.h

OBJS = main.o system.o keyboard.o input.o display.o diskio.o fonts.o         \
       color.o rainbow.o tiles.o tilemap.o audio.o ptplayer.o ui.o           \
			 gameobject.o physics.o level.o anims.o collisions.o									 \
			 display_level.o display_loading.o display_menu.o display_splash.o
################################################################################

# target 'all' (default target)
all : $(EXE)

# $@ matches the target; $< matches the first dependent
main.o : main.c version.h
	$(CC) $< $(CFLAGS)

audio.o : audio.c audio.h settings.h
	$(CC) $< $(CFLAGS)

color.o : color.c color.h settings.h
	$(CC) $< $(CFLAGS)

diskio.o : diskio.c diskio.h settings.h
	$(CC) $< $(CFLAGS)

display_level.o : display_level.c level_display_loop.c level_display_gradients.c level_display_gradients.h display_level.h anims.h collisions.h settings.h
	$(CC) $< $(CFLAGS)

display_loading.o : display_loading.c display_loading.h settings.h
	$(CC) $< $(CFLAGS)

display_menu.o : display_menu.c display_menu.h settings.h
	$(CC) $< $(CFLAGS)

display_splash.o : display_splash.c display_splash.h version.h
	$(CC) $< $(CFLAGS)

fonts.o : fonts.c fonts.h assets.h settings.h
	$(CC) $< $(CFLAGS)

gameobject.o : gameobject.c gameobject.h anims.h collisions.h settings.h
	$(CC) $< $(CFLAGS)

level.o : level.c level.h assets.h settings.h palettes.h
	$(CC) $< $(CFLAGS)

ptplayer.o : ptplayer.asm
	$(AS) $(AFLAGS) -o $@ $<

rainbow.o : rainbow.c rainbow.h settings.h
	$(CC) $< $(CFLAGS)

tiles.o : tiles.c tiles.h settings.h
	$(CC) $< $(CFLAGS)

ui.o : ui.c ui.h
	$(CC) $< $(CFLAGS)

$(EXE) : $(OBJS)
	$(LD) $(EXE) $(OBJS) $(LFLAGS)

# target 'clean'
clean:
	rm -f $(EXE)
	rm -f $(OBJS)
	rm -f ./fonts/fontcache
	find . -name '*.uaem' -delete
