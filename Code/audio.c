///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <hardware/custom.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>

#include "system.h"
#include "audio.h"
///
///globals
extern volatile struct Custom custom;
struct PT_VolumeTable volume_table;
///

///PT_InitPTPlayer()
/******************************************************************************
 * Initializes the volume_table used to animate music volume fade in/out to   *
 * default values.                                                            *
 * And more importantly installs the cia interrupts required by ptplayer.asm. *
 ******************************************************************************/
VOID PT_InitPTPlayer()
{
  //Init volume_table
  volume_table.volume = PTVT_DEFAULT_VOLUME;
  volume_table.state  = PTVT_IDLE;
  volume_table.steps  = PTVT_DEFAULT_STEPS;
  volume_table.step   = 0;

  //Precision value is 24 bits fixed!
  volume_table.increment = ((PTVT_DEFAULT_VOLUME + 1) << 24) / PTVT_DEFAULT_STEPS - 1;
  volume_table.volume_state.value = volume_table.increment * volume_table.step;

  //NOTE: The ptplayer.asm used in this package is modified to aquire VBR by itself
  //      That's why we can safely pass 0 as the second argument below.
  mt_install_cia((void*)&custom, 0, 1);
}
///

///PT_LoadModule(filename)
/******************************************************************************
 * Loads a Protracker Module file from disk.                                  *
 * It will allocate header and patterns on FastMem if available.              *
 ******************************************************************************/
struct PT_Module* PT_LoadModule(STRPTR filename)
{
  BPTR fh;
  ULONG tracker_size = 0;
  struct PT_ModuleHeader header;
  struct PT_Module* mod = AllocMem(sizeof(struct PT_Module), MEMF_ANY | MEMF_CLEAR);

  if (mod) {
    fh = Open(filename, MODE_OLDFILE);
    if (fh) {
      if (Read(fh, &header, sizeof(struct PT_ModuleHeader)) == sizeof(struct PT_ModuleHeader)) {
        if (header.pat_tbl_size <= 128) {
          ULONG i;
          mod->num_patterns = 1;
          for (i = 0; i < header.pat_tbl_size; i++) {
            UBYTE pat_id = header.pat_tbl[i] + 1;
            if (pat_id > mod->num_patterns) mod->num_patterns = pat_id;
          }

          if (mod->num_patterns <= 100) {
            tracker_size = sizeof(struct PT_ModuleHeader) + mod->num_patterns * sizeof(struct PT_Pattern);
            mod->tracker = (struct PT_Tracker*)AllocMem(tracker_size, MEMF_ANY);
            if (mod->tracker) {
              CopyMem(&header, &mod->tracker->header, sizeof(struct PT_ModuleHeader));
              Read(fh, &mod->tracker->pattern, mod->num_patterns * sizeof(struct PT_Pattern));

              for (i = 0; i < 31; i++) {
                mod->samples_size += header.sample_info[i].length_w * 2;
              }

              mod->samples = AllocMem(mod->samples_size, MEMF_CHIP | MEMF_CLEAR);
              if (mod->samples) {
                Read(fh, mod->samples, mod->samples_size);
                Close(fh);
                return mod;
              }
              else puts("Not enough chip memory to load samples!");
            }
            else puts("Not enough memory to load tracker data!");
          }
          else puts("Incompatible number of patterns!");
        }
        else puts("Incompatible module file!");
      }
      else puts("Cannot read module header!");

      Close(fh);
    }
    else puts("Cannot open mod file!");
  }

//Cleanup on error:
  if (mod) {
    if (mod->tracker) FreeMem(mod->tracker, tracker_size);
    if (mod->samples) FreeMem(mod->samples, mod->samples_size);
    FreeMem(mod, sizeof(struct PT_Module));
  }
  return NULL;
}
///
///PT_FreeModule(module)
/******************************************************************************
 * Deallocates a module allocated by PT_LoadModule()                          *
 ******************************************************************************/
VOID PT_FreeModule(struct PT_Module* mod)
{
  if (mod) {
    FreeMem(mod->tracker, sizeof(struct PT_ModuleHeader) + mod->num_patterns * sizeof(struct PT_Pattern));
    FreeMem(mod->samples, mod->samples_size);
    FreeMem(mod, sizeof(struct PT_Module));
  }
}
///

///setVolume(UBYTE vol)
/******************************************************************************
 * Recalculates the volume_table values according to the new volume given.    *
 * vol: has to be from 0 to 64.                                               *
 ******************************************************************************/
VOID setVolume(UBYTE vol)
{
  volume_table.volume = vol;
  volume_table.increment = ((vol + 1) << 24 / volume_table.steps) - 1;
  volume_table.volume_state.value = volume_table.increment * volume_table.step;

  PT_SetModuleVolume(vol);
}
///
///updateVolume()
/******************************************************************************
 * Updates the audio volume according to the audio fade in/out state.         *
 * Designed to be called every frame. Not required to be in VBL.              *
 ******************************************************************************/
VOID updateVolume()
{
  switch (volume_table.state) {
    case PTVT_IDLE:
    return;
    case PTVT_FADE_IN:
      if (volume_table.step == volume_table.steps) {
        volume_table.state = PTVT_IDLE;
        return;
      }
      volume_table.step++;
      volume_table.volume_state.value += volume_table.increment;
    break;
    case PTVT_FADE_OUT:
      if (volume_table.step == 0) {
        volume_table.state = PTVT_IDLE;
        return;
      }
      volume_table.step--;
      volume_table.volume_state.value -= volume_table.increment;
    break;
  }

  PT_SetModuleVolume(volume_table.volume_state.byte.value);
}
///
///changeVolumeSteps(steps)
/******************************************************************************
 * Changes the fade steps of the volume_table. Steps is how many frames the   *
 * fade in/out to/from a master volume will take.                             *
 * WARNING: A value of zero will cause a division by zero error!              *
 ******************************************************************************/
VOID changeVolumeSteps(UWORD steps)
{
  volume_table.step = volume_table.step * steps / volume_table.steps;
  volume_table.steps = steps;

  volume_table.increment = ((volume_table.volume + 1) << 24 / steps) - 1;
  //volume_table.volume_state.value = volume_table.increment * volume_table.step;
}
///
