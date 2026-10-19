#include "types.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "shim/bw_shim.h"

// BW2's sound (system/snd_sys.c and pm_voice.c) as the view calls it: sound effects by BW2's IDs, the battle music and
// Pokémon cries. The sound effects are BW2's own, from its sound archive (BWBATTLE_SOUND_ARCHIVE), played through
// Platinum's sound system: the archive is opened beside Platinum's, with a sound heap of its own, and made the current
// one only while a sound effect loads and starts on Platinum's sound effect players. The cries are Platinum's
// (BwPlat_PlayCry). Without the archive the effects are silent.

#include <stdio.h>
#include <stdlib.h>

#include "bwbattle.h"

// The heap the sound archive's tables and the playing effects' sequences, banks and waves load into
#define BW_SND_HEAP_SIZE 0x400000
// BW2's sound players the effects name (0 to 4, 5 for the sequence's own), each on a handle of its own
#define BW_SND_PLAYERS 6
// Platinum's sound effect players (pl_sound_data.json's PLAYER_SE_1 to PLAYER_SE_4)
#define BW_SND_PLAT_SE_PLAYER_FIRST 3
#define BW_SND_PLAT_SE_PLAYERS 4

static BOOL sSndTried;
static BOOL sSndReady;
static NNSSndArc sSndArc;
static void *sSndHeapMemory;
static NNSSndHeapHandle sSndHeap;
static int sSndHeapEmpty;
static NNSSndHandle sSndHandles[BW_SND_PLAYERS];

static BOOL BwSnd_Init(void) {
    FILE *file;
    NNSSndArc *current;

    if (sSndTried) {
        return sSndReady;
    }
    sSndTried = TRUE;
    file = fopen(BWBATTLE_SOUND_ARCHIVE, "rb");
    if (file == NULL) {
        BwShim_Log("no sound archive (%s): the effects are silent", BWBATTLE_SOUND_ARCHIVE);
        return FALSE;
    }
    fclose(file);
    sSndHeapMemory = malloc(BW_SND_HEAP_SIZE);
    if (sSndHeapMemory == NULL) {
        return FALSE;
    }
    sSndHeap = NNS_SndHeapCreate(sSndHeapMemory, BW_SND_HEAP_SIZE);
    current = NNS_SndArcGetCurrent();
    NNS_SndArcInit(&sSndArc, BWBATTLE_SOUND_ARCHIVE, sSndHeap, FALSE);
    NNS_SndArcSetCurrent(current);
    sSndHeapEmpty = NNS_SndHeapSaveState(sSndHeap);
    for (int i = 0; i < BW_SND_PLAYERS; i++) {
        NNS_SndHandleInit(&sSndHandles[i]);
    }
    sSndReady = TRUE;
    return TRUE;
}

static BOOL BwSnd_AnyPlaying(void) {
    for (int i = 0; i < BW_SND_PLAYERS; i++) {
        if (NNS_SndHandleIsValid(&sSndHandles[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

static void BwSnd_StopAll(void) {
    for (int i = 0; i < BW_SND_PLAYERS; i++) {
        NNS_SndPlayerStopSeq(&sSndHandles[i], 0);
    }
}

static s32 BwSnd_Player(s32 player) {
    return player >= 0 && player < BW_SND_PLAYERS ? player : BW_SND_PLAYERS - 1;
}

static void BwSnd_Play(u32 se, s32 player) {
    NNSSndHandle *handle = &sSndHandles[BwSnd_Player(player)];
    NNSSndArc *current;

    if (!BwSnd_Init()) {
        return;
    }
    NNS_SndPlayerStopSeq(handle, 0);
    // What effects loaded goes once none plays; when the heap is full, the effects still playing stop
    if (!BwSnd_AnyPlaying()) {
        NNS_SndHeapLoadState(sSndHeap, sSndHeapEmpty);
    }
    current = NNS_SndArcGetCurrent();
    NNS_SndArcSetCurrent(&sSndArc);
    if (!NNS_SndArcLoadSeq(se, sSndHeap)) {
        BwSnd_StopAll();
        NNS_SndHeapLoadState(sSndHeap, sSndHeapEmpty);
        if (!NNS_SndArcLoadSeq(se, sSndHeap)) {
            NNS_SndArcSetCurrent(current);
            BwShim_Log("sound effect %u didn't load", se);
            return;
        }
    }
    NNS_SndArcPlayerStartSeqEx(handle, BW_SND_PLAT_SE_PLAYER_FIRST + BwSnd_Player(player) % BW_SND_PLAT_SE_PLAYERS, -1,
                               -1, se);
    NNS_SndArcSetCurrent(current);
}

void GFL_SndSEPlay(u32 se) {
    BwSnd_Play(se, BW_SND_PLAYERS - 1);
}

void GFL_SEPlayKeepVol(u32 se, s32 player) {
    BwSnd_Play(se, player);
}

BOOL GFL_SndPlayerIsActive(s32 player) {
    return sSndReady && NNS_SndHandleIsValid(&sSndHandles[BwSnd_Player(player)]);
}

BOOL GFL_SndPlayerIsActiveAny(void) {
    return sSndReady && BwSnd_AnyPlaying();
}

void GFL_SndPlayerStop(s32 player) {
    if (sSndReady) {
        NNS_SndPlayerStopSeq(&sSndHandles[BwSnd_Player(player)], 0);
    }
}

void GFL_SndPlayerSetParams(s32 player, s32 tempoRatio, s32 pitch, s32 pan) {
    NNSSndHandle *handle = &sSndHandles[BwSnd_Player(player)];

    if (!sSndReady) {
        return;
    }
    if (tempoRatio != -1) {
        NNS_SndPlayerSetTempoRatio(handle, tempoRatio);
    }
    if (pitch != -1) {
        NNS_SndPlayerSetTrackPitch(handle, 0xffff, pitch);
    }
    if (pan != -1) {
        NNS_SndPlayerSetTrackPan(handle, 0xffff, pan);
    }
}

// GFL_SndSEPlay plays on the last handle
s32 GFL_SndSeqGetPlayerIndex(u32 seq) {
    (void)seq;
    return BW_SND_PLAYERS - 1;
}

NNSSndHandle *func_020061a8(s32 player) {
    return &sSndHandles[BwSnd_Player(player)];
}

void GFL_SndStop(void) {
    if (sSndReady) {
        BwSnd_StopAll();
    }
}

// The battle music: Platinum's battle plays its own, so the view's music changes (the low HP music, the trainer's
// last Pokémon) are left to it
void GFL_SndBGMPlay(u32 bgm, u32 channelMask) {
    (void)bgm;
    (void)channelMask;
}

void GFL_SndBGMFadeIn(u16 frames) {
    (void)frames;
}

void GFL_SndBGMFadeOut(u16 frames) {
    (void)frames;
}

u32 GFL_SndBGMGetID(void) {
    return 0;
}

BOOL GFL_SndBGMIsFading(void) {
    return FALSE;
}

void GFL_SndBGMPush(void) {
}

void GFL_SndBGMPop(void) {
}

void GFL_SndBGMSetPaused(BOOL paused) {
    (void)paused;
}

// Cries
void PokeVoice_AllocMulti(int count, HeapID heapId) {
    (void)count;
    (void)heapId;
}

void PokeVoice_FreeMulti(void) {
}

static BOOL sCriesMuted;

void BwBattle_SetCriesMuted(bool muted) {
    sCriesMuted = muted;
}

// BW2's pan is 0 to 127 with the middle at 64, Platinum's -128 to 127
u32 PokeVoice_Play(u32 species, u32 form, u32 pan, u32 chorus, u32 chorusVolume, u32 chorusSpeed, u32 reverse,
                   const PokeVoiceChatterInfo *chatterInfo) {
    (void)chorus;
    (void)chorusVolume;
    (void)chorusSpeed;
    (void)reverse;
    (void)chatterInfo;
    if (!sCriesMuted) {
        BwPlat_PlayCry(species, form, ((int)pan - 64) * 2);
    }
    return 0;
}

BOOL PokeVoice_IsPlaying(u32 handle) {
    (void)handle;
    return BwPlat_IsCryPlaying();
}

void PokeVoice_Release(u32 handle) {
    (void)handle;
}

void PokeVoice_AdjustSpeed(u32 handle, s32 speed) {
    (void)handle;
    (void)speed;
}

void PokeVoice_AdjustVolume(u32 handle, s8 volume) {
    (void)handle;
    (void)volume;
}
