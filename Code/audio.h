#ifndef	AUDIO_H
#define	AUDIO_H

#include "settings.h"

#define PTVT_IDLE     0
#define PTVT_FADE_IN  1
#define PTVT_FADE_OUT 2

#include "ptplayer.h"

struct PT_ModuleHeader {
  UBYTE title[20];
  struct {
    UBYTE name[22];
    UWORD length_w;
    UBYTE finetune;
    UBYTE volume;
    UWORD loop_start_w;
    UWORD loop_length_w;
  } sample_info[31];
  UBYTE pat_tbl_size;
  UBYTE unused;
  UBYTE pat_tbl[128];
  ULONG tracker_id;
};

struct PT_Pattern {
  struct {
    ULONG sample_hi:4;
    ULONG parameter:12;
    ULONG sample_lo:4;
    ULONG effect:12;
  }command[64][4];
};

struct PT_Tracker {
  struct PT_ModuleHeader header;
  struct PT_Pattern pattern;
};

struct PT_Module {
  struct PT_Tracker* tracker;
  UWORD num_patterns;
  APTR samples;
  ULONG samples_size;
};

struct PT_VolumeTable {
  union {
    ULONG value;
    struct {
      UBYTE value;
      UBYTE p1,p2,p3;
    }byte;
  }volume_state;
  ULONG increment;
  UBYTE volume;
  UBYTE state;
  UWORD steps;
  UWORD step;
};

VOID PT_InitPTPlayer(VOID);
struct PT_Module* PT_LoadModule(STRPTR fileName);
VOID PT_FreeModule(struct PT_Module* mod);

#define PT_InitModule(mod, pos) mt_init((void*)&custom, (void*)mod->tracker, (void*)mod->samples, pos)
#define PT_PlayModule() mt_Enable = 1
#define PT_PauseModule() mt_Enable = 0
#define PT_StopAudio() mt_end((void*)&custom)
#define PT_SetModuleVolume(vol) mt_mastervol((void*)&custom, (UBYTE)vol)
#define PT_SetSampleVolume(sample, vol) mt_samplevol((UWORD)sample, (UBYTE)vol)
#define PT_SetModuleChannels(chns) mt_MusicChannels = chns
#define PT_SetChannelMask(mask) mt_channelmask((void*)&custom, (UBYTE)mask)
#define PT_PlaySFX(sfx) mt_playfx((void*)&custom, sfx)
#define PT_LoopSFX(sfx) mt_loopfx((void*)&custom, sfx)
#define PT_StopSFX(cha) mt_stopfx((void*)&custom, cha)
#define PT_TerminatePTPlayer() mt_remove_cia(&custom)

VOID updateVolume(VOID);
VOID setVolume(UBYTE vol);

#endif /* AUDIO_H */
