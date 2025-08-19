/******************************************************************************
 * Sevgi_Engine                                                               *
 * An open source Amiga OCS/ECS/AGA video game engine written entirely* in C. *
 *  Features:                                                                 *
 *  - Directly hits hardware registers for best performance.                  *
 *  - Safe and clean system take over and release back.                       *
 *  - Non blitting sprite engine, with priority, vertical hardware sprite     *
 *    reuse and support for all AGA sprite fetch modes.                       *
 *  - 50FPS, single buffered** 8 way scrollable display which blits only the  *
 *    incoming tiles only once per frame, acoupled with a beam chasing BOB    *
 *    update engine and support for scrollable copper gradients for more than *
 *    one color register.                                                     *
 *    Fast implementations for input from keyboard, two mice and joysticks.   *
 *  - Non-FPU, HUE based fade engine which can also fade copper gradients.    *
 *  - Management of game objects, their images, animations and collisions.    *
 *  - Level based asset loading routines for graphics, audio.                 *
 *  - Support for ILBM multicolor GameFonts.                                  *
 *  - Features "ptplayer" from Frank Wille for music and audio effects.       *
 *  - A UI code capable of creating mouse driven gui objects (buttons, input  *
 *    gadgets, checkboxes etc.) with auto-layout and modern features.         *
 * (*)  Except ptplayer of course. ptplayer is written in 68k Assembly.       *
 * (**) A double buffered display is also possible if one writes their custom *
 *      display_level.c, but this is not encouraged for maximum FPS.          *
 ******************************************************************************/
///defines
//define command line syntax and number of options
#define RDARGS_TEMPLATE ""
#define RDARGS_OPTIONS  0

//#define or #undef GENERATEWBMAIN to enable workbench startup
#define GENERATEWBMAIN
///
///includes
//Amiga headers
#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/icon.h>

//Amiga protos
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/wb.h>

#include <stdio.h>
#include <string.h>

#include "sevgi.h"
#include "version.h"
///
///structs
/***********************************************
* Global configuration struct for this program *
************************************************/
struct Config
{
  struct RDArgs *RDArgs;

  //command line options
  #if RDARGS_OPTIONS
  LONG Options[RDARGS_OPTIONS];
  #endif

  //<YOUR GLOBAL DATA HERE>

};

//<YOUR STRUCTS HERE>

///
///globals
/***********************************************
* Version string for this program              *
************************************************/
#if defined(__SASC)
const UBYTE VersionTag[] = "$VER: " GAME_NAME " " GAME_VERSION_STRING " [" ENGINE_NAME " " ENGINE_VERSION_STRING "] " __AMIGADATE__ "\n\0";
#elif defined(_DCC)
const UBYTE VersionTag[] = "$VER: " GAME_NAME " " GAME_VERSION_STRING " [" ENGINE_NAME " " ENGINE_VERSION_STRING "] (" __COMMODORE_DATE__ ")\n\0";
#elif defined(__GNUC__)
__attribute__((section(".text"))) volatile static const UBYTE VersionTag[] = "$VER: " GAME_NAME " " GAME_VERSION_STRING " [" ENGINE_NAME " " ENGINE_VERSION_STRING "] (" __DATE__ ")\n\0";
#else
const UBYTE VersionTag[] = "$VER: " GAME_NAME " " GAME_VERSION_STRING " [" ENGINE_NAME " " ENGINE_VERSION_STRING "] (" __DATE__ ")\n\0";
#endif

extern ULONG chipset;
///
///prototypes
/***********************************************
* Function forward declarations                *
************************************************/
int            main   (int argc, char** argv);
int            wbmain (struct WBStartup* wbs);
struct Config* Init   (void);
int            Main   (struct Config* config);
void           CleanUp(struct Config* config);
///
///init
/***********************************************
* Program initialization                       *
* - Allocates the config struct to store the   *
*   global configuration data.                 *
* - Do your other initial allocations here.    *
************************************************/
struct Config* Init()
{
  struct Config *config = (struct Config*)AllocMem(sizeof(struct Config), MEMF_CLEAR);

  if (config) {
    //Do your initializations here
  }

  return(config);
}
///
///entry
/***********************************************
 * Ground level entry point                    *
 * - Branches regarding Shell/WB call.         *
 ***********************************************/
int main(int argc, char** argv)
{
  int rc = 20;

  //Early exit if the chipset is not compatible
  detectChipset();
  #ifdef CT_AGA
  if (chipset != CHIPSET_AGA) {
    printf("This game requires AGA chipset!\n");
    return rc;
  }
  #endif

  //argc != 0 identifies call from shell
  if (argc) {
    struct Config *config = Init();

    if (config) {
      #if RDARGS_OPTIONS
        // parse command line arguments
        if ((config->RDArgs = ReadArgs(RDARGS_TEMPLATE, config->Options, NULL)))
          rc = Main(config);
        else
          PrintFault(IoErr(), PROGRAMNAME);
      #else
        rc = Main(config);
      #endif

      CleanUp(config);
    }
  }
  else
    rc = wbmain((struct WBStartup *)argv);

  return(rc);
}

/***********************************************
 * Workbench main                              *
 * - This executable was called from Workbench *
 ***********************************************/
int wbmain(struct WBStartup *wbs)
{
  int rc = 20;

  #ifdef GENERATEWBMAIN
    struct Config *config = Init();

    if (config)
    {
      //<SET Config->Options[] HERE>

      rc = Main(config);

      CleanUp(config);
    }
  #endif

  return(rc);
}
///
///main
/***********************************************
 * Developer level main                        *
 * - Code your program here.                   *
 ***********************************************/
int Main(struct Config* config)
{
  int rc = 0;
  ULONG menuDisplayRetVal;
  BOOL running = TRUE;

  if (takeOverSystem()) {
    if (openFonts()) {
      if (openLoadingDisplay()) {
        showSplashDisplay();

        while (running) {
          menuDisplayRetVal = startMenuDisplay();

          switch (menuDisplayRetVal) {
            case MENU_RV_START:
            startLevelDisplay(1);
            break;
            case MENU_RV_OPTIONS:

            break;
            case MENU_RV_QUIT:
            running = FALSE;
            break;
          }
        }

        closeLoadingDisplay();
      }
      closeFonts();
    }
    giveBackSystem();
  }

  return(rc);
}
///
///cleanup
/***********************************************
 * Clean up before exit                        *
 * - Free allocated resources here.            *
 ***********************************************/
void CleanUp(struct Config* config)
{
  if (config) {
    //<YOUR CLEAN UP CODE HERE>

    // free command line arguments
    #if RDARGS_OPTIONS
      if (config->RDArgs)
        FreeArgs(config->RDArgs);
    #endif

    FreeMem(config, sizeof(struct Config));
  }
}
///
