#include "sound.h"

#include <sys/types.h>
#include <LIBSPU.H>
#include <libcd.h>

#include <stdio.h>

#include "global.h"
#include "cd.h"

#define SOUND_MALLOC_MAX 10
static void play_xa();

char* snd_footstep;
char* snd_engine;

static u_long upload_vag(char *vag, int size, int channel) {
    u_long addr;
    SpuVoiceAttr attributes;

    SpuSetTransferMode(SpuTransByDMA);
    addr = SpuMalloc(size);
    SpuSetTransferStartAddr(addr);
    SpuWrite(vag + 0x30, size);
    SpuIsTransferCompleted(SPU_TRANSFER_WAIT);

    attributes.mask =
        SPU_VOICE_VOLL |
        SPU_VOICE_VOLR |
        SPU_VOICE_PITCH |
        SPU_VOICE_WDSA |
        SPU_VOICE_ADSR_AMODE |
        SPU_VOICE_ADSR_SMODE |
        SPU_VOICE_ADSR_RMODE |
        SPU_VOICE_ADSR_AR |
        SPU_VOICE_ADSR_DR |
        SPU_VOICE_ADSR_SR |
        SPU_VOICE_ADSR_RR |
        SPU_VOICE_ADSR_SL;

    attributes.voice = channel;
    attributes.volume.left  = 0x1fff;
	attributes.volume.right = 0x1fff;
	attributes.pitch        = 0x1000;
	attributes.addr         = addr;
	attributes.a_mode       = SPU_VOICE_LINEARIncN;
	attributes.s_mode       = SPU_VOICE_LINEARIncN;
	attributes.r_mode       = SPU_VOICE_LINEARDecN;
	attributes.ar           = 0x0;
	attributes.dr           = 0x0;
	attributes.sr           = 0x0;
	attributes.rr           = 0x0;
	attributes.sl           = 0xf;

	SpuSetVoiceAttr(&attributes);

    return addr;
}

void init_sound() {
    SpuCommonAttr common;
    u_long size;

    SpuInit();
    SpuInitMalloc(SOUND_MALLOC_MAX, (char*) (SPU_MALLOC_RECSIZ * (SOUND_MALLOC_MAX + 1)));
	common.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR);
	common.mvol.left  = 0x3fff;
	common.mvol.right = 0x3fff;
	SpuSetCommonAttr(&common);

    snd_footstep = cd_read_file("\\FOOTSTEP.VAG;1", &size);
    upload_vag(snd_footstep, size, SPU_0CH);
    //free3(snd_footstep);

    // snd_engine = cd_read_file("\\ENGINE.VAG;1", &size);
    // upload_vag(snd_engine, size, SPU_1CH);

    //play_xa();
}

void set_note(short on, short channel, unsigned short pitch) {
    static SpuVoiceAttr attributes;
    attributes.voice = channel;
    attributes.mask = SPU_VOICE_PITCH;
    attributes.pitch = pitch;
    //printf("%u\n", pitch);
	SpuSetVoiceAttr(&attributes);

    SpuSetKey(on, channel);
}

static void play_xa() {
    CdlFILE file;
    CdlFILTER filter;
    int start, end;
    u_char param[4];
    
    if (!CdSearchFile(&file, "\\ECHOES.XA;1")) {
        printf("failed to find echoes.xa\n");
        return;
    }

    start = CdPosToInt(&file.pos);
    end = start + (file.size / 2048) - 1;

    param[0] = CdlModeSpeed | CdlModeRT | CdlModeSF | CdlModeSize1;
    CdControlB(CdlSetmode, param, 0);
    CdControlF(CdlPause, 0);

    //CdReadyCallback((CdlCB) cb_ready);

    filter.file = 1;
    filter.chan = 0;
    CdControlF(CdlSetfilter, (u_char*) &filter);

    CdControlF(CdlReadS, (u_char*) &file.pos);
}

