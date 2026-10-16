#include "game_opening/opening_starters.h"

#include <nitro.h>
#include <string.h>

#include "constants/species.h"

#include "camera.h"
#include "heap.h"
#include "particle_system.h"
#include "pokemon.h"
#include "pokemon_sprite.h"
#include "rtc.h"
#include "spl.h"
#include "unk_0202419C.h"

#define STARTER_COUNT 3

#define PARTICLE_HEAP_SIZE      0x4800
#define SPRITE_MANAGER_PRIORITY 1023

#define APPEAR_FRAMES       6
#define HOLD_FRAMES         20
#define SPRITE_START_SCALE  0x400
#define SPRITE_NORMAL_SCALE 0x100
#define SPRITE_SCALE_STEP   0x80

enum StarterAppearanceState {
    STARTER_STATE_SHOW = 0,
    STARTER_STATE_ZOOM_IN,
    STARTER_STATE_CREATE_EMITTERS,
    STARTER_STATE_WAIT_FOR_PARTICLES,
    STARTER_STATE_MOVE_OUT,
    STARTER_STATE_HOLD,
};

typedef struct OpeningStarters_t {
    PokemonSpriteManager *spriteManager;
    PokemonSprite *sprites[STARTER_COUNT];
    void *particleHeap;
    ParticleSystem *particleSystem;
    int starterIndex[STARTER_COUNT];
    u8 state;
    u8 timer;
} OpeningStarters;

typedef struct {
    int x;
    int y;
} StarterPosition;

typedef BOOL (*StarterMoveFunc)(OpeningStarters *, const u8);

typedef struct {
    StarterMoveFunc zoomIn;
    StarterMoveFunc moveOut;
} StarterMoves;

static void OpeningStarters_DrawParticles(void);
static void OpeningStarters_EmitterCallback(SPLEmitter *emitter);
static BOOL OpeningStarters_ZoomInTorterra(OpeningStarters *starters, const u8 index);
static BOOL OpeningStarters_MoveOutTorterra(OpeningStarters *starters, const u8 index);
static BOOL OpeningStarters_ZoomInInfernape(OpeningStarters *starters, const u8 index);
static BOOL OpeningStarters_MoveOutInfernape(OpeningStarters *starters, const u8 index);
static BOOL OpeningStarters_ZoomInEmpoleon(OpeningStarters *starters, const u8 index);
static BOOL OpeningStarters_MoveOutEmpoleon(OpeningStarters *starters, const u8 index);

static const StarterPosition sStarterPositions[STARTER_COUNT] = {
    { 0x5C, 0xC0 },
    { 0xE0, 0xC0 },
    { 0x50, 0xC0 }
};

static const StarterMoves sStarterMoves[STARTER_COUNT] = {
    { OpeningStarters_ZoomInTorterra, OpeningStarters_MoveOutTorterra },
    { OpeningStarters_ZoomInInfernape, OpeningStarters_MoveOutInfernape },
    { OpeningStarters_ZoomInEmpoleon, OpeningStarters_MoveOutEmpoleon }
};

static const int sStarterEmitterIDs[STARTER_COUNT][3] = {
    { 6, 7, 8 },
    { 3, 4, 5 },
    { 0, 1, 2 }
};

static u32 OpeningStarters_AllocTextureVram(u32 size, BOOL is4x4Compressed)
{
    u32 address;
    NNSGfdTexKey key;

    key = NNS_GfdAllocTexVram(size, is4x4Compressed, 0);
    ParticleSystem_RegisterTextureKey(key);

    GF_ASSERT(key != NNS_GFD_ALLOC_ERROR_TEXKEY);

    address = NNS_GfdGetTexKeyAddr(key);

    return address;
}

static u32 OpeningStarters_AllocPaletteVram(u32 size, BOOL is4Color)
{
    NNSGfdPlttKey key;
    u32 address;

    key = NNS_GfdAllocPlttVram(size, is4Color, NNS_GFD_ALLOC_FROM_LOW);

    if (key == NNS_GFD_ALLOC_ERROR_PLTTKEY) {
        GF_ASSERT(FALSE);
    }

    ParticleSystem_RegisterPaletteKey(key);

    address = NNS_GfdGetPlttKeyAddr(key);

    return address;
}

int OpeningStarters_GetSize(void)
{
    return sizeof(OpeningStarters);
}

ParticleSystem *OpeningStarters_GetParticleSystem(OpeningStarters *starters)
{
    GF_ASSERT(starters != NULL && starters->particleSystem != NULL);
    return starters->particleSystem;
}

SPLEmitter *OpeningStarters_CreateEmitter(OpeningStarters *starters, int emitterID)
{
    GF_ASSERT(starters != NULL && starters->particleSystem != NULL);

    ParticleSystem_SetCameraProjection(starters->particleSystem, 1);
    return ParticleSystem_CreateEmitterWithCallback(starters->particleSystem, emitterID, NULL, NULL);
}

void OpeningStarters_Init(OpeningStarters *starters)
{
    u8 i;
    PokemonSpriteTemplate spriteTemplate;
    Camera *camera;
    void *resource;
    int species[STARTER_COUNT] = { SPECIES_TORTERRA, SPECIES_INFERNAPE, SPECIES_EMPOLEON };
    int orderedSpecies[STARTER_COUNT];
    int direction, current;
    RTCTime time;

    RTC_GetCurrentTime(&time);

    // The order in which the starters appear depends on the current time
    direction = (time.hour * time.minute + time.second) & 1;
    current = time.second % STARTER_COUNT;

    for (i = 0; i < STARTER_COUNT; i++) {
        orderedSpecies[i] = species[current];
        starters->starterIndex[i] = current;

        if (direction) {
            current++;
        } else {
            current--;
        }

        if (current >= STARTER_COUNT) {
            current = 0;
        } else if (current < 0) {
            current = STARTER_COUNT - 1;
        }
    }

    starters->spriteManager = PokemonSpriteManager_New(HEAP_ID_76);

    for (i = 0; i < STARTER_COUNT; i++) {
        BuildPokemonSpriteTemplate(&spriteTemplate, orderedSpecies[i], 0, 2, 0, 0, 0);
        starters->sprites[i] = PokemonSpriteManager_CreateSprite(starters->spriteManager, &spriteTemplate, sStarterPositions[starters->starterIndex[i]].x, sStarterPositions[starters->starterIndex[i]].y, SPRITE_MANAGER_PRIORITY, i, NULL, NULL);
        PokemonSprite_SetAttribute(starters->sprites[i], MON_SPRITE_HIDE, TRUE);
    }

    ParticleSystem_ZeroAll();

#ifdef SDK_BUILD_ARM
    starters->particleHeap = Heap_Alloc(HEAP_ID_76, PARTICLE_HEAP_SIZE);
    starters->particleSystem = ParticleSystem_New(OpeningStarters_AllocTextureVram, OpeningStarters_AllocPaletteVram, starters->particleHeap, PARTICLE_HEAP_SIZE, 1, HEAP_ID_76);
#else
    starters->particleHeap = Heap_Alloc(HEAP_ID_76, PARTICLE_HEAP_SIZE * 2);
    starters->particleSystem = ParticleSystem_New(OpeningStarters_AllocTextureVram, OpeningStarters_AllocPaletteVram, starters->particleHeap, PARTICLE_HEAP_SIZE * 2, 1, HEAP_ID_76);
#endif

    camera = ParticleSystem_GetCamera(starters->particleSystem);

    Camera_SetClipping(FX32_ONE, FX32_ONE * 900, camera);

    resource = ParticleSystem_LoadResourceFromNARC(NARC_INDEX_PARTICLEDATA__PARTICLEDATA, 4, HEAP_ID_76);
    ParticleSystem_SetResource(starters->particleSystem, resource, (1 << 1) | (1 << 3), 1);
}

void OpeningStarters_Free(OpeningStarters *starters)
{
    ParticleSystem_Free(starters->particleSystem);
    Heap_Free(starters->particleHeap);
    PokemonSpriteManager_Free(starters->spriteManager);
}

void OpeningStarters_UpdateSpriteGraphics(OpeningStarters *starters)
{
    if (starters != NULL) {
        PokemonSpriteManager_UpdateCharAndPltt(starters->spriteManager);
    }
}

void OpeningStarters_Draw(OpeningStarters *starters)
{
    if (starters != NULL) {
        NNS_G3dGeFlushBuffer();
        G3_ResetG3X();
        NNS_G2dSetupSoftwareSpriteCamera();
        PokemonSpriteManager_DrawSprites(starters->spriteManager);
        OpeningStarters_DrawParticles();
        G3_RequestSwapBuffers(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
    }
}

// Runs one starter's whole appearance; returns TRUE once it is finished.
BOOL OpeningStarters_UpdateAppearance(OpeningStarters *starters, const u8 index)
{
    u8 *state = &starters->state;

    switch (*state) {
    case STARTER_STATE_SHOW:
        G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_OBJ, 1);
        PokemonSprite_SetAttribute(starters->sprites[index], MON_SPRITE_SCALE_X, SPRITE_START_SCALE);
        PokemonSprite_SetAttribute(starters->sprites[index], MON_SPRITE_SCALE_Y, SPRITE_START_SCALE);
        PokemonSprite_SetAttribute(starters->sprites[index], MON_SPRITE_HIDE, FALSE);
        (*state)++;
        // fallthrough
    case STARTER_STATE_ZOOM_IN:
        if (sStarterMoves[starters->starterIndex[index]].zoomIn(starters, index)) {
            starters->timer = 0;
            (*state)++;
        }
        break;
    case STARTER_STATE_CREATE_EMITTERS:
        ParticleSystem_SetCameraProjection(starters->particleSystem, 1);
        ParticleSystem_CreateEmitterWithCallback(starters->particleSystem, sStarterEmitterIDs[starters->starterIndex[index]][0], OpeningStarters_EmitterCallback, starters->sprites[index]);
        ParticleSystem_CreateEmitterWithCallback(starters->particleSystem, sStarterEmitterIDs[starters->starterIndex[index]][1], OpeningStarters_EmitterCallback, starters->sprites[index]);
        ParticleSystem_CreateEmitterWithCallback(starters->particleSystem, sStarterEmitterIDs[starters->starterIndex[index]][2], OpeningStarters_EmitterCallback, starters->sprites[index]);
        (*state)++;
        break;
    case STARTER_STATE_WAIT_FOR_PARTICLES:
        if (ParticleSystem_GetActiveEmitterCount(starters->particleSystem) == 0) {
            G2_SetWndOutsidePlane(GX_WND_PLANEMASK_OBJ, 1);
            (*state)++;
        }
        break;
    case STARTER_STATE_MOVE_OUT:
        if (sStarterMoves[starters->starterIndex[index]].moveOut(starters, index)) {
            PokemonSprite_SetAttribute(starters->sprites[index], MON_SPRITE_HIDE, TRUE);
            starters->timer = 0;
            (*state)++;
        }
        break;
    case STARTER_STATE_HOLD:
        starters->timer++;

        if (starters->timer >= HOLD_FRAMES) {
            starters->timer = 0;
            (*state) = STARTER_STATE_SHOW;
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL OpeningStarters_FinishZoomStep(OpeningStarters *starters, PokemonSprite *sprite)
{
    starters->timer++;

    if (starters->timer >= APPEAR_FRAMES) {
        PokemonSprite_SetAttribute(sprite, MON_SPRITE_SCALE_X, SPRITE_NORMAL_SCALE);
        PokemonSprite_SetAttribute(sprite, MON_SPRITE_SCALE_Y, SPRITE_NORMAL_SCALE);
        return TRUE;
    }

    return FALSE;
}

static BOOL OpeningStarters_ZoomInTorterra(OpeningStarters *starters, const u8 index)
{
    PokemonSprite *sprite = starters->sprites[index];

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_X_CENTER, 6);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_Y_CENTER, -16);

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_SCALE_X, -SPRITE_SCALE_STEP);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_SCALE_Y, -SPRITE_SCALE_STEP);

    return OpeningStarters_FinishZoomStep(starters, sprite);
}

static BOOL OpeningStarters_MoveOutTorterra(OpeningStarters *starters, const u8 index)
{
    PokemonSprite *sprite = starters->sprites[index];

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_X_CENTER, -3);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_Y_CENTER, -18);

    return OpeningStarters_FinishZoomStep(starters, sprite);
}

static BOOL OpeningStarters_ZoomInInfernape(OpeningStarters *starters, const u8 index)
{
    PokemonSprite *sprite = starters->sprites[index];

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_X_CENTER, -16);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_Y_CENTER, -16);

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_SCALE_X, -SPRITE_SCALE_STEP);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_SCALE_Y, -SPRITE_SCALE_STEP);

    return OpeningStarters_FinishZoomStep(starters, sprite);
}

static BOOL OpeningStarters_MoveOutInfernape(OpeningStarters *starters, const u8 index)
{
    PokemonSprite *sprite = starters->sprites[index];

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_X_CENTER, -6);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_Y_CENTER, -16);

    return OpeningStarters_FinishZoomStep(starters, sprite);
}

static BOOL OpeningStarters_ZoomInEmpoleon(OpeningStarters *starters, const u8 index)
{
    PokemonSprite *sprite = starters->sprites[index];

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_X_CENTER, 8);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_Y_CENTER, -16);

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_SCALE_X, -SPRITE_SCALE_STEP);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_SCALE_Y, -SPRITE_SCALE_STEP);

    return OpeningStarters_FinishZoomStep(starters, sprite);
}

static BOOL OpeningStarters_MoveOutEmpoleon(OpeningStarters *starters, const u8 index)
{
    PokemonSprite *sprite = starters->sprites[index];

    PokemonSprite_AddAttribute(sprite, MON_SPRITE_X_CENTER, 6);
    PokemonSprite_AddAttribute(sprite, MON_SPRITE_Y_CENTER, -20);

    return OpeningStarters_FinishZoomStep(starters, sprite);
}

static void OpeningStarters_DrawParticles(void)
{
    ParticleSystem_DrawAll();
    ParticleSystem_UpdateAll();
}

static void OpeningStarters_EmitterCallback(SPLEmitter *emitter)
{
    PokemonSprite *sprite = ParticleSystem_GetEmitterCallbackParam();
    VecFx32 position = { 0, 0, 0 };

    position.x = 172 * (PokemonSprite_GetAttribute(sprite, MON_SPRITE_X_CENTER) - 128);
    position.y = 172 * (PokemonSprite_GetAttribute(sprite, MON_SPRITE_Y_CENTER) - 96);
    position.z = 0x40;

    SPLEmitter_SetPos(emitter, &position);
}
