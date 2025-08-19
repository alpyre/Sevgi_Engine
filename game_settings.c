/******************************************************************************
 * GameSettings                                                               *
 ******************************************************************************/
///Defines
#define MUIM_GameSettings_SetDepthLimits    0x80430810
#define MUIM_GameSettings_SetPF2DepthLimits 0x80430811

#define NO_VALUE 0x7FFFFFFF
#define IS_VALUE TRUE
#define IS_BOOL  FALSE
#define WriteSaveString(s) Write(fh, save_strings[s], strlen(save_strings[s]))
#define WriteString(s) Write(fh, s, strlen(s))
///
///Includes
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>    // <-- Required for tag redirection

#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h> // <-- Required for DoSuperMethod()
#include <clib/alib_stdio_protos.h> // <-- Required for sprintf()

#include <SDI_compiler.h>     //     Required for
#include <SDI_hook.h>         // <-- multi platform
#include <SDI_stdarg.h>       //     compatibility

#include "dosupernew.h"
#include "utility.h"
#include "integer_gadget.h"
#include "game_settings.h"
///
///Structs
struct cl_ObjTable
{
  Object* grp_video;
  Object* chk_AGA;
  Object* chk_dynamicCL;
  Object* chk_smartSprites;
  Object* chk_dualplayfield;
  Object* cyc_videoSystem;
  Object* int_scrWidth;
  Object* int_scrHeight;
  Object* int_scrDepth;
  Object* int_scrPf2Depth;
  Object* int_scrPf2Height;
  Object* int_scrTopPanelHeight;
  Object* int_scrBottomPanelHeight;
  Object* int_bplFetch;
  Object* int_sprFetch;
  Object* int_gfxFade;
  Object* cyc_paula;
  Object* int_musVolume;
  Object* int_musFade;
  Object* int_sfxVolume;
  Object* int_sfxChan;
  Object* int_sfxPriority;
  Object* chk_small;
  Object* chk_big;
  Object* chk_hitbox_small;
  Object* int_numBOBs;
  Object* int_numSprites;
  Object* int_numGameObjects;
};

struct cl_Data
{
  struct cl_ObjTable obj_table;
  BOOL custom_level_display;
  BOOL edited;
};

struct cl_Msg
{
  ULONG MethodID;
  STRPTR filename;
  ULONG num_levels;
  ULONG num_textfonts;
  ULONG num_gamefonts;
};
///
///Globals
extern APTR g_MemoryPool;
extern struct FileRequester* g_FileReq;

extern Object* App;
extern struct MUI_CustomClass* MUIC_Integer;
extern struct MUI_CustomClass* MUIC_ImageDisplay;

STRPTR paula_cycles[] = {"PAL", "NTSC", NULL};

STRPTR save_strings[] = {
"#ifndef SETTINGS_H\n#define SETTINGS_H\n",
"\n#endif /* SETTINGS_H */\n",
"\n/******************************************************************************\n",
" ******************************************************************************/\n\n",
" * VIDEO                                                                      *\n",
" * AUDIO                                                                      *\n",
" * FONTS                                                                      *\n",
" * GAMEOBJECTS                                                                *\n",
"#ifdef CUSTOM_LEVEL_DISPLAY\n#include \"custom_display_settings.h\"\n#else\n",
"#endif //CUSTOM_LEVEL_DISPLAY\n",
"//Un-comment to activate 24bit AGA color features.",
"//Set the default fade in/out speed. Smaller is faster. (2-256)",
"//Un-comment to activate per-frame dynamic copperlist generation.",
"//Un-comment to activate smart sprite algorithms.",
"//Un-comment to activate dual-playfield mode.",
"//Select bitplane and sprite fetch modes",
"   // Bitplane fetch mode (1, 2 or 4)",
"   // Sprite fetch mode (1, 2 or 4)",
"    // Visible resolution of the level screen",
"   // \"       \"          \"  \"   \"     \"",
"     // How many bitplanes (1-6 on OCS/ECS, 1-8 on AGA)",
"//Set these to valid values in DUALPLAYFIELD mode",
"//Un-comment if you wrote a custom display_level.c which is not compatible with Sevgi_Editor",
"//Set PAL/NTSC",
"//Set the default module playback volume. (0-64)",
"//Set the default audio fade in/out speed. Smaller is faster. (1-256)",
"//Default playback volume for sound samples (0-64)",
"//Default playback channel for sound samples (0-3 / -1 for best avail. channel)",
"//Default playback priority for sound samples (1-127)",
" // Number of levels in the game",
" // Number of Amiga TextFonts to load",
" // Number of GameFonts to load",
"#define SCR_WIDTH_EXTRA  64  // Additional horizontal pixels for scroll\n",
"#define SCR_HEIGHT_EXTRA 32  // Additional vertical pixels for scroll (2 extra tiles)\n",
"#define PAULA_PAL_CYCLES  3546895\n#define PAULA_NTSC_CYCLES 3579545\n#define PAULA_MIN_PERIOD  124\n\n",
"#define TF_DEFAULT 0   // Index of the default TextFont\n\n",
"#define GF_DEFAULT 0   // Index of the first loaded GameFont\n",
"CT_AGA",
"CT_DEFAULT_STEPS ",
"DYNAMIC_COPPERLIST",
"SMART_SPRITES",
"DUALPLAYFIELD",
"BPL_FMODE ",
"SPR_FMODE ",
"TOP_PANEL_HEIGHT ",
"BOTTOM_PANEL_HEIGHT ",
"SCREEN_WIDTH ",
"SCREEN_HEIGHT ",
"SCREEN_DEPTH ",
"BITMAP_DEPTH_PF2 ",
"BITMAP_HEIGHT_PF2 ",
"CUSTOM_LEVEL_DISPLAY",
"PAULA_CYCLES ",
"PAULA_PAL_CYCLES",
"PAULA_NTSC_CYCLES",
"PTVT_DEFAULT_VOLUME ",
"PTVT_DEFAULT_STEPS ",
"SFX_DEFAULT_VOLUME ",
"SFX_DEFAULT_CHANNEL ",
"SFX_DEFAULT_PRIORITY ",
"NUM_LEVELS ",
"NUM_TEXTFONTS ",
"NUM_GAMEFONTS ",
"NUM_SPRITES ",
"NUM_BOBS ",
"NUM_GAMEOBJECTS ",
"SMALL_IMAGE_SIZES",
"BIG_IMAGE_SIZES",
"SMALL_HITBOX_SIZES"
};

enum {
  SS_HEADER,
  SS_FOOTER,
  SS_RECT_OPEN,
  SS_RECT_CLOSE,
  SS_VIDEO,
  SS_AUDIO,
  SS_FONTS,
  SS_GAMEOBJECTS,
  SS_CUSTOM_LEVEL_DISPLAY_START,
  SS_CUSTOM_LEVEL_DISPLAY_END,
  SS_COMMENT_AGA,
  SS_COMMENT_VIDEO_FADE,
  SS_COMMENT_DYNAMIC_CL,
  SS_COMMENT_SMART_SPRITES,
  SS_COMMENT_DUALPLAYFIELD,
  SS_COMMENT_FETCH_MODES,
  SS_COMMENT_BPL_FETCH,
  SS_COMMENT_SPR_FETCH,
  SS_COMMENT_WIDTH,
  SS_COMMENT_HEIGHT,
  SS_COMMENT_DEPTH,
  SS_COMMENT_BITMAP_DEPTH_PF2,
  SS_COMMENT_CUSTOM_LEVEL_DISPLAY,
  SS_COMMENT_AUDIO_PAL_NTSC,
  SS_COMMENT_AUDIO_DEFAULT_MUS_VOL,
  SS_COMMENT_AUDIO_FADE,
  SS_COMMENT_AUDIO_DEFAULT_SFX_VOL,
  SS_COMMENT_AUDIO_DEFAULT_SFX_CHANNEL,
  SS_COMMENT_AUDIO_DEFAULT_SFX_PRIORITY,
  SS_COMMENT_NUM_LEVELS,
  SS_COMMENT_NUM_TEXTFONTS,
  SS_COMMENT_NUM_GAMEFONTS,
  SS_SCR_WIDTH_EXTRA,
  SS_SCR_HEIGHT_EXTRA,
  SS_PAULA_VALUES,
  SS_TF_DEFAULT,
  SS_GF_DEFAULT,
  SS_DEF_CT_AGA,
  SS_DEF_CT_DEFAULT_STEPS,
  SS_DEF_DYNAMIC_COPPERLIST,
  SS_DEF_SMART_SPRITES,
  SS_DEF_DUALPLAYFIELD,
  SS_DEF_BPL_FMODE,
  SS_DEF_SPR_FMODE,
  SS_DEF_TOP_PANEL_HEIGHT,
  SS_DEF_BOTTOM_PANEL_HEIGHT,
  SS_DEF_SCREEN_WIDTH,
  SS_DEF_SCREEN_HEIGHT,
  SS_DEF_SCREEN_DEPTH,
  SS_DEF_BITMAP_DEPTH_PF2,
  SS_DEF_BITMAP_HEIGHT_PF2,
  SS_DEF_CUSTOM_LEVEL_DISPLAY,
  SS_DEF_PAULA_CYCLES,
  SS_DEF_PAULA_PAL_CYCLES,
  SS_DEF_PAULA_NTSC_CYCLES,
  SS_DEF_PTVT_DEFAULT_VOLUME,
  SS_DEF_PTVT_DEFAULT_STEPS,
  SS_DEF_SFX_DEFAULT_VOLUME,
  SS_DEF_SFX_DEFAULT_CHANNEL,
  SS_DEF_SFX_DEFAULT_PRIORITY,
  SS_DEF_NUM_LEVELS,
  SS_DEF_NUM_TEXTFONTS,
  SS_DEF_NUM_GAMEFONTS,
  SS_DEF_NUM_SPRITES,
  SS_DEF_NUM_BOBS,
  SS_DEF_NUM_GAMEOBJECTS,
  SS_DEF_SMALL_IMAGE_SIZES,
  SS_DEF_BIG_IMAGE_SIZES,
  SS_DEF_SMALL_HITBOX_SIZES
};

static struct {
  STRPTR AGA;
  STRPTR dynamic_cl;
  STRPTR smart_spr;
  STRPTR dualplayfield;
  STRPTR video_system;
  STRPTR scr_width;
  STRPTR scr_height;
  STRPTR scr_depth;
  STRPTR scr_pf2_depth;
  STRPTR scr_pf2_height;
  STRPTR top_panel_height;
  STRPTR bottom_panel_height;
  STRPTR bpl_fetch;
  STRPTR spr_fetch;
  STRPTR gfx_fade;
  STRPTR paula_cycles;
  STRPTR mus_volume;
  STRPTR mus_fade_speed;
  STRPTR sfx_volume;
  STRPTR sfx_channel;
  STRPTR sfx_priority;
  STRPTR small_sizes;
  STRPTR big_sizes;
  STRPTR small_hitbox_sizes;
  STRPTR num_bobs;
  STRPTR num_sprites;
  STRPTR num_gameobjects;
}help_string = {
  "Select this to activate AGA features.",
  "Activates Dynamic Copperlist mode in level display.",
  "Activates Smart Sprites mode in in level display.",
  "Activates dualplayfield mode on the level display.",
  "Select video system.\nNTSC is NOT IMPLEMENTED YET!",
  "Screen width for the level display in pixels.\nValues other than 320 is NOT IMPLEMENTED YET!",
  "Screen height for the level display in pixels.\nThis value is the height of the tile map display.\nDoes not include top and bottom panels heights.",
  "Color depth of the level display.\nThis should be compatible with the color depth of the tilesets.\n\nNOTE: This becomes the color depth of the foreground,\ntilemap (playfield 1) on dualplayfield mode.",
  "Color depth of the parallaxing background bitmap on dualplayfield mode.\nThis should be comptible with the ILBM image loaded for the background.",
  "Height of the parallaxing background bitmap on dualplayfield mode.\nThis sould be compatible with the ILBM image loaded for the background.",
  "Height of the stationary top panel for level display\nwhich could be used as a dashboard to display lives,\nscores etc. A value of 0 disables it.",
  "Height of the stationary bottom panel for level display\nwhich could be used as a dashboard to display lives,\nscores etc. A value of 0 disables it.",
  "Fetch mode for bitplanes on level display.\nOnly available on AGA machines.\nValues more than 1 will lower available hardware sprites.",
  "Fetch mode for sprites on level display.\nOnly available on AGA machines.\nHigher fetch modes will make wider hardware sprites possible.\nThe value here should be compatible with the sprite banks used.",
  "Color fade in/out speed.\nHigher values will make the fade longer.",
  "Affects the playback pitch of sound samples.",
  "Default maximum volume for module playback.",
  "Music fade in/out speed.\nHigher values will make the fade longer.",
  "Default sound sample playback volume.",
  "Default audio channel to playback sound samples.\nA value of -1 auto-selects most available channel.",
  "Default playback priority for all loaded sound samples.\nYou should set a higher priority than this to the special\nsound effects which would over play other sound effects\nmanually in your code.",
  "Unset this if the image sizes of any of your sprites/BOBs exceeds 256 pixels.\nThe value here must be compatible with the values used creating your\nSpriteBanks/BobSheets.",
  "Set this if the hotspot offset of any of your sprites/BOBs exceeds +/-128.\nThe value here must be compatible with the values used creating your\nSpriteBanks/BobSheets",
  "Set this if none of the hitbox offsets in your sprites/BOBs exceed +/-128.\nThe value here must be compatible with the values used creating your\nSpriteBanks/BobSheets",
  "Maximum number of BOBs to be displayed\nsimultaneously on the level display.",
  "Maximum number of Sprite images to be displayed\nsimultaneously on the level display.",
  "Number of spawnable GameObjects."
};
///

///writeSetting(file_handle, intro_comment, define, string, value, outro_comment, comment_out, separate)
STATIC VOID writeSetting(BPTR fh, ULONG intro_comment, ULONG define, ULONG string, LONG value, ULONG outro_comment, BOOL comment_out, BOOL separate)
{
  UBYTE buffer[128];

  if (intro_comment) {
    WriteString(save_strings[intro_comment]);
    Write(fh, "\n", 1);
  }

  if (comment_out) Write(fh, "//", 2);

  Write(fh, "#define ", 8);
  WriteString(save_strings[define]);

  if (string) {
    WriteString(save_strings[string]);
  }
  else if (value != NO_VALUE) {
    sprintf(buffer, "%ld", value);
    WriteString(buffer);
  }

  if (outro_comment) WriteString(save_strings[outro_comment]);

  if (separate) Write(fh, "\n\n", 2);
  else Write(fh, "\n", 1);
}
///
///readSetting(buffer, define, is_value)
LONG readSetting(UBYTE* buffer, ULONG define, BOOL is_value)
{
  LONG retval = 0;

  if (define == SS_DEF_PAULA_CYCLES) {
    ULONG pos = 0;

    if (searchString(buffer, save_strings[define], &pos)) {
      pos += strlen(save_strings[define]);
      while (buffer[pos] == ' ' || buffer[pos] == 9 /*tab*/) pos++;

      if (!strncmp(&buffer[pos], "PAULA_NTSC_CYCLES", 17)) {
        retval = 1;
      }
      else retval = 0;
    }
    else retval = 0;
  }
  else {
    if (is_value) {
      ULONG pos = 0;

      if (searchString(buffer, save_strings[define], &pos)) {
        pos += strlen(save_strings[define]);
        aToi(&buffer[pos], &retval);
      }
    }
    else {
      ULONG pos = 0;

      if (searchString(buffer, save_strings[define], &pos)) {
        BOOL commented = FALSE;
        //go back until finding the newline
        while (pos) {
          if (buffer[--pos] == 10) break;
          if (buffer[pos] == '/') {
            commented = TRUE;
            break;
          }
        }

        retval = !commented;
      }
      else retval = 0;
    }
  }

  return retval;
}
///

///m_SetPF2DepthLimits()
STATIC ULONG m_SetPF2DepthLimits(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG dualplayfield;

  get(data->obj_table.chk_dualplayfield, MUIA_Selected, &dualplayfield);

  if (dualplayfield) {
    ULONG scr_depth;

    get(data->obj_table.int_scrDepth, MUIA_Integer_Value, &scr_depth);

    if (scr_depth == 1) {
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Integer_Min, 1);
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Integer_Max, 1);
    }
    else {
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Integer_Min, scr_depth - 1);
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Integer_Max, scr_depth);
    }
  }

  return 0;
}
///
///m_SetDepthLimits()
STATIC ULONG m_SetDepthLimits(struct IClass* cl, Object* obj, Msg msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  ULONG aga;
  ULONG dualplayfield;

  get(data->obj_table.chk_AGA, MUIA_Selected, &aga);
  get(data->obj_table.chk_dualplayfield, MUIA_Selected, &dualplayfield);

  if (aga) {
    if (dualplayfield) {
      DoMethod(data->obj_table.int_scrDepth, MUIM_NoNotifySet, MUIA_Integer_Max, 4);
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
      DoMethod(data->obj_table.int_scrPf2Height, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
      DoMethod(data->obj_table.int_bplFetch, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.int_bplFetch, MUIM_NoNotifySet, MUIA_Integer_Value, 1);
      m_SetPF2DepthLimits(cl, obj, msg);
    }
    else {
      DoMethod(data->obj_table.int_scrDepth, MUIM_NoNotifySet, MUIA_Integer_Max, 8);
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.int_scrPf2Height, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.int_bplFetch, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
    }

    DoMethod(data->obj_table.int_sprFetch, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
  }
  else {
    if (dualplayfield) {
      DoMethod(data->obj_table.int_scrDepth, MUIM_NoNotifySet, MUIA_Integer_Max, 3);
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
      DoMethod(data->obj_table.int_scrPf2Height, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
      m_SetPF2DepthLimits(cl, obj, msg);
    }
    else {
      DoMethod(data->obj_table.int_scrDepth, MUIM_NoNotifySet, MUIA_Integer_Max, 6);
      DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
      DoMethod(data->obj_table.int_scrPf2Height, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
    }

    DoMethod(data->obj_table.int_bplFetch, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.int_sprFetch, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
    DoMethod(data->obj_table.int_bplFetch, MUIM_NoNotifySet, MUIA_Integer_Value, 1);
    DoMethod(data->obj_table.int_sprFetch, MUIM_NoNotifySet, MUIA_Integer_Value, 1);
  }

  return 0;
}
///
///m_Load()
STATIC ULONG m_Load(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  UBYTE* buffer;
  struct FileInfoBlock fib;

  if (msg->filename) {
    BPTR lock = Lock(msg->filename, ACCESS_READ);
    if (lock) {
      Examine(lock, &fib);
      UnLock(lock);
    }
    else return 0;

    buffer = AllocMem(fib.fib_Size, MEMF_ANY);
    if (buffer) {
      BPTR fh = Open(msg->filename, MODE_OLDFILE);

      if (fh) {
        if (Read(fh, buffer, fib.fib_Size) > 0) {
          ULONG dynamic_cl;

          data->custom_level_display = readSetting(buffer, SS_DEF_CUSTOM_LEVEL_DISPLAY, IS_BOOL);
          DoMethod(data->obj_table.grp_video, MUIM_Set, MUIA_Disabled, data->custom_level_display);

          DoMethod(data->obj_table.chk_AGA, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_CT_AGA, IS_BOOL));
          DoMethod(data->obj_table.chk_dynamicCL, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_DYNAMIC_COPPERLIST, IS_BOOL));
          DoMethod(data->obj_table.chk_smartSprites, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_SMART_SPRITES, IS_BOOL));
          DoMethod(data->obj_table.chk_dualplayfield, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_DUALPLAYFIELD, IS_BOOL));
          m_SetDepthLimits(cl, obj, (Msg) msg);
          m_SetPF2DepthLimits(cl, obj, (Msg) msg);
          DoMethod(data->obj_table.int_scrWidth, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SCREEN_WIDTH, IS_VALUE));
          DoMethod(data->obj_table.int_scrHeight, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SCREEN_HEIGHT, IS_VALUE));
          DoMethod(data->obj_table.int_scrDepth, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SCREEN_DEPTH, IS_VALUE));
          DoMethod(data->obj_table.int_scrPf2Depth, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_BITMAP_DEPTH_PF2, IS_VALUE));
          DoMethod(data->obj_table.int_scrPf2Height, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_BITMAP_HEIGHT_PF2, IS_VALUE));
          DoMethod(data->obj_table.int_scrTopPanelHeight, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_TOP_PANEL_HEIGHT, IS_VALUE));
          DoMethod(data->obj_table.int_scrBottomPanelHeight, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_BOTTOM_PANEL_HEIGHT, IS_VALUE));
          DoMethod(data->obj_table.int_bplFetch, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_BPL_FMODE, IS_VALUE));
          DoMethod(data->obj_table.int_sprFetch, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SPR_FMODE, IS_VALUE));
          DoMethod(data->obj_table.int_gfxFade, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_CT_DEFAULT_STEPS, IS_VALUE));

          DoMethod(data->obj_table.cyc_paula, MUIM_NoNotifySet, MUIA_Cycle_Active, readSetting(buffer, SS_DEF_PAULA_CYCLES, IS_BOOL));
          DoMethod(data->obj_table.int_musVolume, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_PTVT_DEFAULT_VOLUME, IS_VALUE));
          DoMethod(data->obj_table.int_musFade, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_PTVT_DEFAULT_STEPS, IS_VALUE));
          DoMethod(data->obj_table.int_sfxVolume, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SFX_DEFAULT_VOLUME, IS_VALUE));
          DoMethod(data->obj_table.int_sfxChan, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SFX_DEFAULT_CHANNEL, IS_VALUE));
          DoMethod(data->obj_table.int_sfxPriority, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_SFX_DEFAULT_PRIORITY, IS_VALUE));
          DoMethod(data->obj_table.chk_small, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_SMALL_IMAGE_SIZES, IS_BOOL));
          DoMethod(data->obj_table.chk_big, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_BIG_IMAGE_SIZES, IS_BOOL));
          DoMethod(data->obj_table.chk_hitbox_small, MUIM_NoNotifySet, MUIA_Selected, readSetting(buffer, SS_DEF_SMALL_HITBOX_SIZES, IS_BOOL));
          DoMethod(data->obj_table.int_numBOBs, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_NUM_BOBS, IS_VALUE));
          DoMethod(data->obj_table.int_numSprites, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_NUM_SPRITES, IS_VALUE));
          DoMethod(data->obj_table.int_numGameObjects, MUIM_NoNotifySet, MUIA_Integer_Value, readSetting(buffer, SS_DEF_NUM_GAMEOBJECTS, IS_VALUE));

          //Enable/disable bound objects without triggering edited state
          get(data->obj_table.chk_dynamicCL, MUIA_Selected, &dynamic_cl);
          if (dynamic_cl) {
            DoMethod(data->obj_table.chk_smartSprites, MUIM_NoNotifySet, MUIA_Disabled, FALSE);
          }
          else {
            DoMethod(data->obj_table.chk_smartSprites, MUIM_NoNotifySet, MUIA_Selected, FALSE);
            DoMethod(data->obj_table.chk_smartSprites, MUIM_NoNotifySet, MUIA_Disabled, TRUE);
          }

          data->edited = FALSE;
        }

        Close(fh);
      }
      FreeMem(buffer, fib.fib_Size);
    }
  }

  return 0;
}
///
///m_Save()
STATIC ULONG m_Save(struct IClass* cl, Object* obj, struct cl_Msg* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  BPTR fh;
  struct {
    ULONG AGA;
    ULONG dynamic_cl;
    ULONG smart_spr;
    ULONG dualplayfield;
    ULONG video_system;
    ULONG screen_width;
    ULONG screen_height;
    ULONG screen_depth;
    ULONG screen_pf2_depth;
    ULONG screen_pf2_height;
    ULONG top_panel_height;
    ULONG bottom_panel_height;
    ULONG bpl_fetch;
    ULONG spr_fetch;
    ULONG vid_fade_speed;
    ULONG paula_cycles;
    ULONG mus_volume;
    ULONG mus_fade_speed;
    ULONG sfx_volume;
    LONG  sfx_channel;
    ULONG sfx_priority;
    ULONG small_sizes;
    ULONG big_sizes;
    ULONG small_hitbox_sizes;
    ULONG max_bobs;
    ULONG max_sprites;
    ULONG num_gameobjects;
  }settings;

  fh = Open(msg->filename, MODE_READWRITE);
  if (fh) {
    get(data->obj_table.chk_AGA, MUIA_Selected, &settings.AGA);
    get(data->obj_table.chk_dynamicCL, MUIA_Selected, &settings.dynamic_cl);
    get(data->obj_table.chk_smartSprites, MUIA_Selected, &settings.smart_spr);
    get(data->obj_table.chk_dualplayfield, MUIA_Selected, &settings.dualplayfield);
    get(data->obj_table.cyc_videoSystem, MUIA_Cycle_Active, &settings.video_system);
    get(data->obj_table.int_scrWidth, MUIA_Integer_Value, &settings.screen_width);
    get(data->obj_table.int_scrHeight, MUIA_Integer_Value, &settings.screen_height);
    get(data->obj_table.int_scrDepth, MUIA_Integer_Value, &settings.screen_depth);
    get(data->obj_table.int_scrPf2Depth, MUIA_Integer_Value, &settings.screen_pf2_depth);
    get(data->obj_table.int_scrPf2Height, MUIA_Integer_Value, &settings.screen_pf2_height);
    get(data->obj_table.int_scrTopPanelHeight, MUIA_Integer_Value, &settings.top_panel_height);
    get(data->obj_table.int_scrBottomPanelHeight, MUIA_Integer_Value, &settings.bottom_panel_height);
    get(data->obj_table.int_bplFetch, MUIA_Integer_Value, &settings.bpl_fetch);
    get(data->obj_table.int_sprFetch, MUIA_Integer_Value, &settings.spr_fetch);
    get(data->obj_table.int_gfxFade, MUIA_Integer_Value, &settings.vid_fade_speed);
    get(data->obj_table.cyc_paula, MUIA_Cycle_Active, &settings.paula_cycles);
    get(data->obj_table.int_musVolume, MUIA_Integer_Value, &settings.mus_volume);
    get(data->obj_table.int_musFade, MUIA_Integer_Value, &settings.mus_fade_speed);
    get(data->obj_table.int_sfxVolume, MUIA_Integer_Value, &settings.sfx_volume);
    get(data->obj_table.int_sfxChan, MUIA_Integer_Value, &settings.sfx_channel);
    get(data->obj_table.int_sfxPriority, MUIA_Integer_Value, &settings.sfx_priority);
    get(data->obj_table.chk_small, MUIA_Selected, &settings.small_sizes);
    get(data->obj_table.chk_big, MUIA_Selected, &settings.big_sizes);
    get(data->obj_table.chk_hitbox_small, MUIA_Selected, &settings.small_hitbox_sizes);
    get(data->obj_table.int_numBOBs, MUIA_Integer_Value, &settings.max_bobs);
    get(data->obj_table.int_numSprites, MUIA_Integer_Value, &settings.max_sprites);
    get(data->obj_table.int_numGameObjects, MUIA_Integer_Value, &settings.num_gameobjects);

    WriteSaveString(SS_HEADER);
    WriteSaveString(SS_RECT_OPEN);
    WriteSaveString(SS_VIDEO);
    WriteSaveString(SS_RECT_CLOSE);

    writeSetting(fh, SS_COMMENT_CUSTOM_LEVEL_DISPLAY, SS_DEF_CUSTOM_LEVEL_DISPLAY, NULL, NO_VALUE, NULL, !data->custom_level_display, TRUE);

    WriteSaveString(SS_CUSTOM_LEVEL_DISPLAY_START);

    writeSetting(fh, SS_COMMENT_AGA, SS_DEF_CT_AGA, NULL, NO_VALUE, NULL, !settings.AGA, FALSE);
    writeSetting(fh, SS_COMMENT_VIDEO_FADE, SS_DEF_CT_DEFAULT_STEPS, NULL, settings.vid_fade_speed, NULL, FALSE, TRUE);
    writeSetting(fh, SS_COMMENT_DYNAMIC_CL, SS_DEF_DYNAMIC_COPPERLIST, NULL, NO_VALUE, NULL, !settings.dynamic_cl, FALSE);
    writeSetting(fh, SS_COMMENT_SMART_SPRITES, SS_DEF_SMART_SPRITES, NULL, NO_VALUE, NULL, !settings.smart_spr, FALSE);
    writeSetting(fh, SS_COMMENT_DUALPLAYFIELD, SS_DEF_DUALPLAYFIELD, NULL, NO_VALUE, NULL, !settings.dualplayfield,FALSE);
    writeSetting(fh, SS_COMMENT_FETCH_MODES, SS_DEF_BPL_FMODE, NULL, settings.bpl_fetch, SS_COMMENT_BPL_FETCH, FALSE, FALSE);
    writeSetting(fh, NULL, SS_DEF_SPR_FMODE, NULL, settings.spr_fetch, SS_COMMENT_SPR_FETCH, FALSE, TRUE);

    writeSetting(fh, NULL, SS_DEF_TOP_PANEL_HEIGHT, NULL, settings.top_panel_height, NULL, FALSE, FALSE);
    writeSetting(fh, NULL, SS_DEF_BOTTOM_PANEL_HEIGHT, NULL, settings.bottom_panel_height, NULL, FALSE, TRUE);

    writeSetting(fh, NULL, SS_DEF_SCREEN_WIDTH, NULL, settings.screen_width, SS_COMMENT_WIDTH, FALSE, FALSE);
    writeSetting(fh, NULL, SS_DEF_SCREEN_HEIGHT, NULL, settings.screen_height, SS_COMMENT_HEIGHT, FALSE, FALSE);
    WriteSaveString(SS_SCR_WIDTH_EXTRA);
    WriteSaveString(SS_SCR_HEIGHT_EXTRA);
    writeSetting(fh, NULL, SS_DEF_SCREEN_DEPTH, NULL, settings.screen_depth, SS_COMMENT_DEPTH, FALSE, TRUE);

    writeSetting(fh, SS_COMMENT_BITMAP_DEPTH_PF2, SS_DEF_BITMAP_DEPTH_PF2, NULL, settings.screen_pf2_depth, NULL, FALSE, FALSE);
    writeSetting(fh, NULL, SS_DEF_BITMAP_HEIGHT_PF2, NULL, settings.screen_pf2_height, NULL, FALSE, TRUE);

    WriteSaveString(SS_CUSTOM_LEVEL_DISPLAY_END);

    WriteSaveString(SS_RECT_OPEN);
    WriteSaveString(SS_AUDIO);
    WriteSaveString(SS_RECT_CLOSE);

    WriteSaveString(SS_PAULA_VALUES);

    writeSetting(fh, SS_COMMENT_AUDIO_PAL_NTSC, SS_DEF_PAULA_CYCLES, settings.paula_cycles ? SS_DEF_PAULA_NTSC_CYCLES : SS_DEF_PAULA_PAL_CYCLES, 0, NULL, FALSE, FALSE);
    writeSetting(fh, SS_COMMENT_AUDIO_DEFAULT_MUS_VOL, SS_DEF_PTVT_DEFAULT_VOLUME, NULL, settings.mus_volume, NULL, FALSE, FALSE);
    writeSetting(fh, SS_COMMENT_AUDIO_FADE, SS_DEF_PTVT_DEFAULT_STEPS, NULL, settings.mus_fade_speed, NULL, FALSE, FALSE);
    writeSetting(fh, SS_COMMENT_AUDIO_DEFAULT_SFX_VOL, SS_DEF_SFX_DEFAULT_VOLUME, NULL, settings.sfx_volume, NULL, FALSE, FALSE);
    writeSetting(fh, SS_COMMENT_AUDIO_DEFAULT_SFX_CHANNEL, SS_DEF_SFX_DEFAULT_CHANNEL, NULL, settings.sfx_channel, NULL, FALSE, FALSE);
    writeSetting(fh, SS_COMMENT_AUDIO_DEFAULT_SFX_PRIORITY, SS_DEF_SFX_DEFAULT_PRIORITY, NULL, settings.sfx_priority, NULL, FALSE, FALSE);

    WriteSaveString(SS_RECT_OPEN);
    WriteSaveString(SS_FONTS);
    WriteSaveString(SS_RECT_CLOSE);

    writeSetting(fh, NULL, SS_DEF_NUM_TEXTFONTS, NULL, msg->num_textfonts, SS_COMMENT_NUM_TEXTFONTS, FALSE, FALSE);
    WriteSaveString(SS_TF_DEFAULT);

    writeSetting(fh, NULL, SS_DEF_NUM_GAMEFONTS, NULL, msg->num_gamefonts, SS_COMMENT_NUM_GAMEFONTS, FALSE, FALSE);
    WriteSaveString(SS_GF_DEFAULT);

    WriteSaveString(SS_RECT_OPEN);
    WriteSaveString(SS_GAMEOBJECTS);
    WriteSaveString(SS_RECT_CLOSE);

    writeSetting(fh, NULL, SS_DEF_NUM_LEVELS, NULL, msg->num_levels, SS_COMMENT_NUM_LEVELS, FALSE, TRUE);

    writeSetting(fh, NULL, SS_DEF_NUM_SPRITES, NULL, settings.max_sprites, NULL, FALSE, FALSE);
    writeSetting(fh, NULL, SS_DEF_NUM_BOBS, NULL, settings.max_bobs, NULL, FALSE, FALSE);
    writeSetting(fh, NULL, SS_DEF_NUM_GAMEOBJECTS, NULL, settings.num_gameobjects, NULL, FALSE, TRUE);

    writeSetting(fh, NULL, SS_DEF_SMALL_IMAGE_SIZES, NULL, NO_VALUE, NULL, !settings.small_sizes, FALSE);
    writeSetting(fh, NULL, SS_DEF_BIG_IMAGE_SIZES, NULL, NO_VALUE, NULL, !settings.big_sizes, FALSE);
    writeSetting(fh, NULL, SS_DEF_SMALL_HITBOX_SIZES, NULL, NO_VALUE, NULL, !settings.small_hitbox_sizes, FALSE);

    WriteSaveString(SS_FOOTER);

    SetFileSize(fh, 0, OFFSET_CURRENT);
    Close(fh);

    data->edited = FALSE;
  }

  return 0;
}
///

///Overridden OM_NEW
static ULONG m_New(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data;
  struct {
    Object* grp_video;
    Object* chk_AGA;
    Object* chk_dynamicCL;
    Object* chk_smartSprites;
    Object* chk_dualplayfield;
    Object* cyc_videoSystem;
    Object* int_scrWidth;
    Object* int_scrHeight;
    Object* int_scrDepth;
    Object* int_scrPf2Depth;
    Object* int_scrPf2Height;
    Object* int_scrTopPanelHeight;
    Object* int_scrBottomPanelHeight;
    Object* int_bplFetch;
    Object* int_sprFetch;
    Object* int_gfxFade;
    Object* cyc_paula;
    Object* int_musVolume;
    Object* int_musFade;
    Object* int_sfxVolume;
    Object* int_sfxChan;
    Object* int_sfxPriority;
    Object* chk_small;
    Object* chk_big;
    Object* chk_hitbox_small;
    Object* int_numBOBs;
    Object* int_numSprites;
    Object* int_numGameObjects;
  }objects;

  if ((obj = (Object *) DoSuperNew(cl, obj,
    MUIA_Window_ID, MAKE_ID('S','V','G','2'),
    MUIA_Window_Width, MUIV_Window_Width_Visible(56),
    MUIA_Window_Height, MUIV_Window_Height_Visible(52),
    MUIA_Window_Title, "Game Settings",
    MUIA_Window_RootObject, MUI_NewObject(MUIC_Group,
      MUIA_Group_Horiz, TRUE,
      MUIA_Group_Child, (objects.grp_video = MUI_NewObject(MUIC_Group,
        MUIA_FrameTitle, "Video",
        MUIA_Frame, MUIV_Frame_Group,
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_AGA, FALSE, "AGA", 'a', help_string.AGA),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_dynamicCL, FALSE, "Dynamic Copperlist", 'c', help_string.dynamic_cl),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_smartSprites, FALSE, "Smart Sprites", 's', help_string.smart_spr),
        MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_dualplayfield, FALSE, "Dualplayfield", 'd', help_string.dualplayfield),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Group_Columns, 2,
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Video System:",
            MUIA_ShortHelp, help_string.video_system,
          TAG_END),
          MUIA_Group_Child, (objects.cyc_videoSystem = MUI_NewObject(MUIC_Cycle,
            MUIA_Cycle_Active, 0,
            MUIA_Cycle_Entries, paula_cycles,
            MUIA_ShortHelp, help_string.video_system,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Screen Width:",
            MUIA_ShortHelp, help_string.scr_width,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrWidth = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, FALSE,
            MUIA_Integer_Value, 320,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 320,
            MUIA_Integer_Max, 320,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.scr_width,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Screen Height:",
            MUIA_ShortHelp, help_string.scr_height,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrHeight = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 256,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 16,
            MUIA_Integer_Max, 256,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.scr_height,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Screen Depth:",
            MUIA_ShortHelp, help_string.scr_depth,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrDepth = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 6,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 1,
            MUIA_Integer_Max, 6,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.scr_depth,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "  Playfield 2 Depth:",
            MUIA_ShortHelp, help_string.scr_pf2_depth,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrPf2Depth = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 3,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 1,
            MUIA_Integer_Max, 3,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.scr_pf2_depth,
            MUIA_Disabled, TRUE,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "  Playfield 2 Height:",
            MUIA_ShortHelp, help_string.scr_pf2_height,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrPf2Height = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 256,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 16,
            MUIA_Integer_Max, 1024,
            MUIA_String_MaxLen, 5,
            MUIA_ShortHelp, help_string.scr_pf2_height,
            MUIA_Disabled, TRUE,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Top Panel Height:",
            MUIA_ShortHelp, help_string.top_panel_height,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrTopPanelHeight = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 208,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.top_panel_height,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Bottom Panel Height:",
            MUIA_ShortHelp, help_string.bottom_panel_height,
          TAG_END),
          MUIA_Group_Child, (objects.int_scrBottomPanelHeight = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 0,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 0,
            MUIA_Integer_Max, 208,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.bottom_panel_height,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Bitplane Fetch Mode:",
            MUIA_ShortHelp, help_string.bpl_fetch,
          TAG_END),
          MUIA_Group_Child, (objects.int_bplFetch = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_124, TRUE,
            MUIA_Disabled, TRUE,
            MUIA_ShortHelp, help_string.bpl_fetch,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Sprite Fetch Mode:",
            MUIA_ShortHelp, help_string.spr_fetch,
          TAG_END),
          MUIA_Group_Child, (objects.int_sprFetch = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_124, TRUE,
            MUIA_Disabled, TRUE,
            MUIA_ShortHelp, help_string.spr_fetch,
          TAG_END)),
          MUIA_Group_Child, MUI_NewObject(MUIC_Text,
            MUIA_Text_Contents, "Fade In/Out Speed:",
            MUIA_ShortHelp, help_string.gfx_fade,
          TAG_END),
          MUIA_Group_Child, (objects.int_gfxFade = NewObject(MUIC_Integer->mcc_Class, NULL,
            MUIA_Integer_Input, TRUE,
            MUIA_Integer_Value, 64,
            MUIA_Integer_Incr, 1,
            MUIA_Integer_Buttons, TRUE,
            MUIA_Integer_Min, 2,
            MUIA_Integer_Max, 256,
            MUIA_String_MaxLen, 4,
            MUIA_ShortHelp, help_string.gfx_fade,
          TAG_END)),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
      TAG_END)),
      MUIA_Group_Child, MUI_NewObject(MUIC_Group,
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_FrameTitle, "Audio",
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Columns, 2,
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Paula Cycles:",
              MUIA_ShortHelp, help_string.paula_cycles,
            TAG_END),
            MUIA_Group_Child, (objects.cyc_paula = MUI_NewObject(MUIC_Cycle,
              MUIA_Cycle_Active, 0,
              MUIA_Cycle_Entries, paula_cycles,
              MUIA_ShortHelp, help_string.paula_cycles,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Default Music Volume:",
              MUIA_ShortHelp, help_string.mus_volume,
            TAG_END),
            MUIA_Group_Child, (objects.int_musVolume = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 64,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 64,
              MUIA_String_MaxLen, 3,
              MUIA_ShortHelp, help_string.mus_volume,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Default Fade Speed:",
              MUIA_ShortHelp, help_string.mus_fade_speed,
            TAG_END),
            MUIA_Group_Child, (objects.int_musFade = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 64,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 1,
              MUIA_Integer_Max, 64,
              MUIA_String_MaxLen, 3,
              MUIA_ShortHelp, help_string.mus_fade_speed,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Default SFX Volume:",
              MUIA_ShortHelp, help_string.sfx_volume,
            TAG_END),
            MUIA_Group_Child, (objects.int_sfxVolume = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 64,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 64,
              MUIA_String_MaxLen, 3,
              MUIA_ShortHelp, help_string.sfx_volume,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Default SFX Channel:",
              MUIA_ShortHelp, help_string.sfx_channel,
            TAG_END),
            MUIA_Group_Child, (objects.int_sfxChan = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, -1,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, -1,
              MUIA_Integer_Max, 3,
              MUIA_String_MaxLen, 3,
              MUIA_ShortHelp, help_string.sfx_channel,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Default SFX Priority:",
              MUIA_ShortHelp, help_string.sfx_priority,
            TAG_END),
            MUIA_Group_Child, (objects.int_sfxPriority = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 1,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 1,
              MUIA_Integer_Max, 127,
              MUIA_String_MaxLen, 4,
              MUIA_ShortHelp, help_string.sfx_priority,
            TAG_END)),
          TAG_END),
        TAG_END),
        MUIA_Group_Child, MUI_NewObject(MUIC_Group,
          MUIA_Frame, MUIV_Frame_Group,
          MUIA_FrameTitle, "Gameobjects",
          MUIA_Group_Child, MUI_NewObject(MUIC_Group,
            MUIA_Group_Columns, 2,
            MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_small, TRUE, "Small sizes", 0, help_string.small_sizes),
            MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_hitbox_small, TRUE, "Small hitbox sizes", 0, help_string.small_hitbox_sizes),
            MUIA_Group_Child, MUI_NewCheckMark(&objects.chk_big, FALSE, "Big sizes", 0, help_string.big_sizes),
            MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Max. BOBs:",
              MUIA_ShortHelp, help_string.num_bobs,
            TAG_END),
            MUIA_Group_Child, (objects.int_numBOBs = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 10,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 128,
              MUIA_String_MaxLen, 4,
              MUIA_ShortHelp, help_string.num_bobs,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Max. Sprites:",
              MUIA_ShortHelp, help_string.num_sprites,
            TAG_END),
            MUIA_Group_Child, (objects.int_numSprites = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 8,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 64,
              MUIA_String_MaxLen, 4,
              MUIA_ShortHelp, help_string.num_sprites,
            TAG_END)),
            MUIA_Group_Child, MUI_NewObject(MUIC_Text,
              MUIA_Text_Contents, "Num. Gameobjects:",
              MUIA_ShortHelp, help_string.num_gameobjects,
            TAG_END),
            MUIA_Group_Child, (objects.int_numGameObjects = NewObject(MUIC_Integer->mcc_Class, NULL,
              MUIA_Integer_Input, TRUE,
              MUIA_Integer_Value, 10,
              MUIA_Integer_Incr, 1,
              MUIA_Integer_Buttons, TRUE,
              MUIA_Integer_Min, 0,
              MUIA_Integer_Max, 256,
              MUIA_String_MaxLen, 4,
              MUIA_ShortHelp, help_string.num_gameobjects,
            TAG_END)),
          TAG_END),
          //MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle, TAG_END),
        TAG_END),
      TAG_END),
    TAG_END),
    TAG_MORE, msg->ops_AttrList)))
  {
    data = (struct cl_Data *) INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Window_SetCycleChain, objects.chk_AGA,
                                             objects.chk_dynamicCL,
                                             objects.chk_smartSprites,
                                             objects.chk_dualplayfield,
                                             objects.cyc_videoSystem,
                                             MUI_GetChild(objects.int_scrWidth, 1),
                                             MUI_GetChild(objects.int_scrHeight, 1),
                                             MUI_GetChild(objects.int_scrDepth, 1),
                                             MUI_GetChild(objects.int_scrPf2Depth, 1),
                                             MUI_GetChild(objects.int_scrPf2Height, 1),
                                             MUI_GetChild(objects.int_scrTopPanelHeight, 1),
                                             MUI_GetChild(objects.int_scrBottomPanelHeight, 1),
                                             MUI_GetChild(objects.int_bplFetch, 2),
                                             MUI_GetChild(objects.int_bplFetch, 3),
                                             MUI_GetChild(objects.int_sprFetch, 2),
                                             MUI_GetChild(objects.int_sprFetch, 3),
                                             MUI_GetChild(objects.int_gfxFade, 1),
                                             objects.cyc_paula,
                                             MUI_GetChild(objects.int_musVolume, 1),
                                             MUI_GetChild(objects.int_musFade, 1),
                                             MUI_GetChild(objects.int_sfxVolume, 1),
                                             MUI_GetChild(objects.int_sfxChan, 1),
                                             MUI_GetChild(objects.int_sfxPriority, 1),
                                             objects.chk_small,
                                             objects.chk_big,
                                             objects.chk_hitbox_small,
                                             MUI_GetChild(objects.int_numBOBs, 1),
                                             MUI_GetChild(objects.int_numSprites, 1),
                                             MUI_GetChild(objects.int_numGameObjects, 1),
                                             NULL);

    DoMethod(objects.chk_smartSprites, MUIM_Set, MUIA_Disabled, TRUE);

    DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3,
      MUIM_Set, MUIA_Window_Open, FALSE);

    DoMethod(objects.cyc_videoSystem, MUIM_Notify, MUIA_Cycle_Active, 0, objects.int_scrHeight, 3,
      MUIM_Set, MUIA_Integer_Max, 256);

    DoMethod(objects.cyc_videoSystem, MUIM_Notify, MUIA_Cycle_Active, 1, objects.int_scrHeight, 3,
      MUIM_Set, MUIA_Integer_Max, 200);

    DoMethod(objects.cyc_videoSystem, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, objects.cyc_paula, 3,
      MUIM_Set, MUIA_Cycle_Active, MUIV_TriggerValue);

    DoMethod(objects.chk_AGA, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1,
      MUIM_GameSettings_SetDepthLimits);

    DoMethod(objects.chk_dualplayfield, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 1,
      MUIM_GameSettings_SetDepthLimits);

    DoMethod(objects.chk_dynamicCL, MUIM_Notify, MUIA_Selected, FALSE, objects.chk_smartSprites, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(objects.chk_dynamicCL, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, objects.chk_smartSprites, 3,
      MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue);

    DoMethod(objects.int_scrDepth, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 1,
      MUIM_GameSettings_SetPF2DepthLimits);

    DoMethod(objects.chk_small, MUIM_Notify, MUIA_Selected, TRUE, objects.chk_big, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    DoMethod(objects.chk_big, MUIM_Notify, MUIA_Selected, TRUE, objects.chk_small, 3,
      MUIM_Set, MUIA_Selected, FALSE);

    /**************************************************************************/
    //A value change on all these gadgets has to set 'edited' state TRUE
    DoMethod(objects.cyc_videoSystem, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.chk_AGA, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.chk_dynamicCL, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.chk_smartSprites, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_scrWidth, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_scrHeight, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_scrDepth, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_scrTopPanelHeight, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_scrBottomPanelHeight, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_bplFetch, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_sprFetch, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_gfxFade, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_numBOBs, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_numSprites, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_numGameObjects, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.cyc_paula, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_musVolume, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_musFade, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_sfxVolume, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_sfxChan, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.int_sfxPriority, MUIM_Notify, MUIA_Integer_Value, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.chk_small, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.chk_big, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);

    DoMethod(objects.chk_hitbox_small, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 3,
    	MUIM_Set, MUIA_GameSettings_Edited, TRUE);
    /**************************************************************************/

    data->custom_level_display = FALSE;
    data->edited = FALSE;
    data->obj_table.grp_video = objects.grp_video;
    data->obj_table.cyc_videoSystem = objects.cyc_videoSystem;
    data->obj_table.chk_AGA = objects.chk_AGA;
    data->obj_table.chk_dynamicCL = objects.chk_dynamicCL;
    data->obj_table.chk_smartSprites = objects.chk_smartSprites;
    data->obj_table.chk_dualplayfield = objects.chk_dualplayfield;
    data->obj_table.int_scrWidth = objects.int_scrWidth;
    data->obj_table.int_scrHeight = objects.int_scrHeight;
    data->obj_table.int_scrDepth = objects.int_scrDepth;
    data->obj_table.int_scrPf2Depth = objects.int_scrPf2Depth;
    data->obj_table.int_scrPf2Height = objects.int_scrPf2Height;
    data->obj_table.int_scrTopPanelHeight = objects.int_scrTopPanelHeight;
    data->obj_table.int_scrBottomPanelHeight = objects.int_scrBottomPanelHeight;
    data->obj_table.int_bplFetch = objects.int_bplFetch;
    data->obj_table.int_sprFetch = objects.int_sprFetch;
    data->obj_table.int_gfxFade = objects.int_gfxFade;
    data->obj_table.int_numBOBs = objects.int_numBOBs;
    data->obj_table.int_numSprites = objects.int_numSprites;
    data->obj_table.int_numGameObjects = objects.int_numGameObjects;

    data->obj_table.cyc_paula = objects.cyc_paula;
    data->obj_table.int_musVolume = objects.int_musVolume;
    data->obj_table.int_musFade = objects.int_musFade;
    data->obj_table.int_sfxVolume = objects.int_sfxVolume;
    data->obj_table.int_sfxChan = objects.int_sfxChan;
    data->obj_table.int_sfxPriority = objects.int_sfxPriority;

    data->obj_table.chk_small = objects.chk_small;
    data->obj_table.chk_big = objects.chk_big;
    data->obj_table.chk_hitbox_small = objects.chk_hitbox_small;

    //<SUBCLASS INITIALIZATION HERE>
    if (/*<Success of your initializations>*/ TRUE) {

      return((ULONG) obj);
    }
    else CoerceMethod(cl, obj, OM_DISPOSE);
  }

return (ULONG) NULL;
}
///
///Overridden OM_DISPOSE
static ULONG m_Dispose(struct IClass* cl, Object* obj, Msg msg)
{
  //struct cl_Data *data = INST_DATA(cl, obj);

  //<FREE SUBCLASS INITIALIZATIONS HERE>

  return DoSuperMethodA(cl, obj, msg);
}
///
///Overridden OM_SET
//*****************
static ULONG m_Set(struct IClass* cl, Object* obj, struct opSet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);
  struct TagItem *tags, *tag;

  for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
  {
    switch (tag->ti_Tag)
    {
      //<SUBCLASS ATTRIBUTES HERE>
      case MUIA_GameSettings_Edited:
        data->edited = tag->ti_Data;
      break;
    }
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Overridden OM_GET
//*****************
static ULONG m_Get(struct IClass* cl, Object* obj, struct opGet* msg)
{
  struct cl_Data *data = INST_DATA(cl, obj);

  switch (msg->opg_AttrID)
  {
    //<SUBCLASS ATTRIBUTES HERE>
    case MUIA_GameSettings_Edited:
      *msg->opg_Storage = data->edited;
    return TRUE;
    case MUIA_GameSettings_AGACheck:
      *msg->opg_Storage = (ULONG)data->obj_table.chk_AGA;
    return TRUE;
  }

  return (DoSuperMethodA(cl, obj, (Msg) msg));
}
///
///Dispatcher
SDISPATCHER(cl_Dispatcher)
{
  struct cl_Data *data;
  if (! (msg->MethodID == OM_NEW)) data = INST_DATA(cl, obj);

  switch(msg->MethodID)
  {
    case OM_NEW:
      return m_New(cl, obj, (struct opSet*) msg);
    case OM_DISPOSE:
      return m_Dispose(cl, obj, msg);
    case OM_SET:
      return m_Set(cl, obj, (struct opSet*) msg);
    case OM_GET:
      return m_Get(cl, obj, (struct opGet*) msg);
    case MUIM_GameSettings_Save:
      return m_Save(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GameSettings_Load:
      return m_Load(cl, obj, (struct cl_Msg*) msg);
    case MUIM_GameSettings_SetDepthLimits:
      return m_SetDepthLimits(cl, obj, msg);
    case MUIM_GameSettings_SetPF2DepthLimits:
      return m_SetPF2DepthLimits(cl, obj, msg);

    //<DISPATCH SUBCLASS METHODS HERE>

    default:
      return DoSuperMethodA(cl, obj, msg);
  }
}
///
///Class Creator
struct MUI_CustomClass* MUI_Create_GameSettings(void)
{
    return (MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct cl_Data), ENTRY(cl_Dispatcher)));
}
///
