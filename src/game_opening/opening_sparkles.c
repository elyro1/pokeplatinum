#include "game_opening/opening_sparkles.h"

#include <nitro.h>
#include <string.h>

#include "game_opening/opening_timeline.h"

#include "char_transfer.h"
#include "gx_layers.h"
#include "heap.h"
#include "math_util.h"
#include "pltt_transfer.h"
#include "render_oam.h"
#include "sprite.h"
#include "sprite_resource.h"
#include "sprite_transfer.h"
#include "sprite_util.h"
#include "sys_task.h"
#include "sys_task_manager.h"

#define SPRITE_RESOURCE_TYPE_COUNT 4

// The sparkle entry lookup only ever searches the first 10 entries, even for
// the twinkle pool, which holds more sprites than that.
#define MAX_ACTIVE_SPARKLES 10

#define SPARKLE_FALL_FRAMES   80
#define SPARKLE_FALL_SPEED    3
#define TWINKLE_LIFETIME      8
#define SPARKLE_TASK_PRIORITY 20

typedef struct {
    u8 inUse;
    u8 timer;
    Sprite *sprite;
    SysTask *task;
} SparkleEntry;

typedef struct TwinklePool_t {
    int count;
    SparkleEntry *entries;
} TwinklePool;

typedef struct {
    u8 sparklesPerBurst;
    u8 burstInterval;
    u8 burstCount;
    u8 burstsDone;
    u8 timer;
    u8 type;
} SparkleSpawnParams;

typedef struct {
    SparkleSpawnParams params;
    SysTask *task;
    SparkleEntry *entries;
} SparkleSpawner;

typedef struct {
    SpriteList *spriteList;
    G2dRenderer renderer;
    SpriteResourceCollection *collections[SPRITE_RESOURCE_TYPE_COUNT];
    SpriteResource *resources[SPRITE_RESOURCE_TYPE_COUNT];
    SparkleEntry entries[MAX_ACTIVE_SPARKLES];
} LogoSparkleGraphics;

typedef struct LogoSparkles_t {
    LogoSparkleGraphics graphics;
    SparkleSpawner spawners[3];
} LogoSparkles;

static void LogoSparkles_InitGraphics(LogoSparkleGraphics *graphics);
static void LogoSparkles_FreeGraphics(LogoSparkleGraphics *graphics);
static void LogoSparkles_CreateSprites(LogoSparkleGraphics *graphics);
static void LogoSparkles_UpdateSprites(LogoSparkleGraphics *graphics);
static void LogoSparkles_StartSpawner(SparkleSpawner *spawner, LogoSparkles *sparkles, const u8 sparklesPerBurst, const u8 burstInterval, const u8 burstCount, const u8 type);
static void SparkleSpawner_Task(SysTask *task, void *param);
static SparkleEntry *SparkleEntry_Acquire(SparkleEntry *entries);
static void SparkleEntry_FallTask(SysTask *task, void *param);
static void TwinklePool_SpawnTwinkles(TwinklePool *pool, const u8 count, BOOL hidden);
static void SparkleEntry_TwinkleTask(SysTask *task, void *param);

static const u8 sSpriteResourceCounts[SPRITE_RESOURCE_TYPE_COUNT] = {
    1,
    1,
    1,
    1
};

static void LogoSparkles_InitTransfers(void)
{
    const int heapID = HEAP_ID_76;

    {
        CharTransferTemplate template = {
            1, 0, 0, heapID
        };

        CharTransfer_Init(&template);
    }

    PlttTransfer_Init(1, heapID);
    CharTransfer_ClearBuffers();
    PlttTransfer_Clear();
}

static void LogoSparkles_InitSpriteTemplate(LogoSparkleGraphics *graphics, AffineSpriteListTemplate *template, SpriteResourcesHeader *header)
{
    SpriteResourcesHeader_Init(header, 0, 0, 0, 0, 0xffffffff, 0xffffffff, 0, 0, graphics->collections[0], graphics->collections[1], graphics->collections[2], graphics->collections[3], NULL, NULL);

    template->list = graphics->spriteList;
    template->resourceData = header;
    template->position.x = 0;
    template->position.y = 0;
    template->position.z = 0;
    template->affineScale.x = FX32_ONE;
    template->affineScale.y = FX32_ONE;
    template->affineScale.z = FX32_ONE;
    template->affineZRotation = 0;
    template->priority = 1;
    template->vramType = NNS_G2D_VRAM_TYPE_2DMAIN;
    template->heapID = HEAP_ID_76;
}

LogoSparkles *LogoSparkles_New(void)
{
    int size = sizeof(LogoSparkles);
    LogoSparkles *sparkles = Heap_Alloc(HEAP_ID_76, size);

    memset(sparkles, 0, size);

    LogoSparkles_InitGraphics(&sparkles->graphics);
    LogoSparkles_CreateSprites(&sparkles->graphics);

    return sparkles;
}

TwinklePool *TwinklePool_New(const int count)
{
    int entriesSize;
    TwinklePool *pool = Heap_Alloc(HEAP_ID_76, sizeof(TwinklePool));
    entriesSize = sizeof(SparkleEntry) * count;
    pool->entries = Heap_Alloc(HEAP_ID_76, entriesSize);

    memset(pool->entries, 0, entriesSize);
    pool->count = count;

    return pool;
}

void TwinklePool_Free(TwinklePool *pool)
{
    int i;

    for (i = 0; i < pool->count; i++) {
        if (pool->entries[i].task != NULL) {
            SysTask_Done(pool->entries[i].task);
        }
    }

    Heap_Free(pool->entries);
    Heap_Free(pool);
}

Sprite **TwinklePool_GetSpriteSlot(TwinklePool *pool, const int index)
{
    GF_ASSERT(index < pool->count);
    return &pool->entries[index].sprite;
}

void LogoSparkles_Free(LogoSparkles *sparkles)
{
    u8 i;

    LogoSparkles_FreeGraphics(&sparkles->graphics);

    for (i = 0; i < 3; i++) {
        if (sparkles->spawners[i].task != NULL) {
            SysTask_Done(sparkles->spawners[i].task);
        }
    }

    for (i = 0; i < MAX_ACTIVE_SPARKLES; i++) {
        if (sparkles->graphics.entries[i].task != NULL) {
            SysTask_Done(sparkles->graphics.entries[i].task);
        }
    }

    Heap_Free(sparkles);
}

static void LogoSparkles_InitGraphics(LogoSparkleGraphics *graphics)
{
    int i;
    const int heapID = HEAP_ID_76;

    GX_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);
    GXS_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);

    LogoSparkles_InitTransfers();

    NNS_G2dInitOamManagerModule();
    RenderOam_Init(0, 128, 0, 32, 0, 128, 0, 32, heapID);

    graphics->spriteList = SpriteList_InitRendering(MAX_ACTIVE_SPARKLES, &graphics->renderer, heapID);

    for (i = 0; i < SPRITE_RESOURCE_TYPE_COUNT; i++) {
        graphics->collections[i] = SpriteResourceCollection_New(sSpriteResourceCounts[i], i, heapID);
    }

    graphics->resources[0] = SpriteResourceCollection_AddTiles(graphics->collections[0], 128, 4, 0, 0, NNS_G2D_VRAM_TYPE_2DBOTH, heapID);
    graphics->resources[1] = SpriteResourceCollection_AddPalette(graphics->collections[1], 128, 3, 0, 0, NNS_G2D_VRAM_TYPE_2DBOTH, 1, heapID);
    graphics->resources[2] = SpriteResourceCollection_Add(graphics->collections[2], 128, 5, 0, 0, 2, heapID);
    graphics->resources[3] = SpriteResourceCollection_Add(graphics->collections[3], 128, 6, 0, 0, 3, heapID);

    SpriteTransfer_RequestChar(graphics->resources[0]);
    SpriteTransfer_RequestPlttWholeRange(graphics->resources[1]);

    GXLayers_EngineAToggleLayers(GX_PLANEMASK_OBJ, 1);
    GXLayers_EngineBToggleLayers(GX_PLANEMASK_OBJ, 1);
}

static void LogoSparkles_FreeGraphics(LogoSparkleGraphics *graphics)
{
    u8 i;

    SpriteTransfer_ResetCharTransfer(graphics->resources[0]);
    SpriteTransfer_ResetPlttTransfer(graphics->resources[1]);

    for (i = 0; i < SPRITE_RESOURCE_TYPE_COUNT; i++) {
        SpriteResourceCollection_Delete(graphics->collections[i]);
    }

    SpriteList_Delete(graphics->spriteList);
    RenderOam_Free();
    CharTransfer_Free();
    PlttTransfer_Free();
}

static void LogoSparkles_CreateSprites(LogoSparkleGraphics *graphics)
{
    int i;
    SpriteResourcesHeader header;
    AffineSpriteListTemplate template;

    for (i = 0; i < MAX_ACTIVE_SPARKLES; i++) {
        LogoSparkles_InitSpriteTemplate(graphics, &template, &header);
        template.position.x = FX32_ONE * i * 2;
        template.position.y = FX32_ONE * i * 2;

        graphics->entries[i].sprite = SpriteList_AddAffine(&template);

        Sprite_SetAnimateFlag(graphics->entries[i].sprite, FALSE);
        Sprite_SetDrawFlag(graphics->entries[i].sprite, FALSE);
    }
}

static void LogoSparkles_UpdateSprites(LogoSparkleGraphics *graphics)
{
    SpriteList_Update(graphics->spriteList);
}

void LogoSparkles_Update(LogoSparkles *sparkles, const int frame)
{
    if (frame == LOGO_FRAME_FIRST_SPARKLES) {
        LogoSparkles_StartSpawner(&sparkles->spawners[0], sparkles, 3, 3, 3, 0);
    } else if (frame == LOGO_FRAME_SECOND_SPARKLES) {
        LogoSparkles_StartSpawner(&sparkles->spawners[1], sparkles, 3, 3, 3, 1);
    }

    LogoSparkles_UpdateSprites(&sparkles->graphics);
}

static void LogoSparkles_StartSpawner(SparkleSpawner *spawner, LogoSparkles *sparkles, const u8 sparklesPerBurst, const u8 burstInterval, const u8 burstCount, const u8 type)
{
    spawner->params.sparklesPerBurst = sparklesPerBurst;
    spawner->params.burstInterval = burstInterval;
    spawner->params.burstCount = burstCount;
    spawner->params.timer = 0;
    spawner->params.burstsDone = 0;
    spawner->entries = sparkles->graphics.entries;
    spawner->params.type = type;
    spawner->task = SysTask_Start(SparkleSpawner_Task, spawner, 10);
}

static void LogoSparkles_SpawnSparkles(SparkleEntry *entries, const u8 count, const u8 type)
{
    u8 i;
    SparkleEntry *entry;
    int baseY;

    if (type == 0) {
        baseY = 64;
    } else {
        baseY = 32;
    }

    for (i = 0; i < count; i++) {
        entry = SparkleEntry_Acquire(entries);

        {
            VecFx32 position;

            position = *Sprite_GetPosition(entry->sprite);
            position.x = FX32_ONE * (64 + (LCRNG_Next() % 128));
            position.y = FX32_ONE * (baseY + (LCRNG_Next() % 64));

            Sprite_SetPosition(entry->sprite, &position);
        }

        Sprite_SetAnimateFlag(entry->sprite, TRUE);
        Sprite_SetDrawFlag(entry->sprite, TRUE);

        entry->task = SysTask_Start(SparkleEntry_FallTask, entry, SPARKLE_TASK_PRIORITY);
    }
}

static SparkleEntry *SparkleEntry_Acquire(SparkleEntry *entries)
{
    u8 i;
    SparkleEntry *entry;

    for (i = 0; i < MAX_ACTIVE_SPARKLES; i++) {
        if (!entries[i].inUse) {
            entry = &entries[i];
            entry->inUse = TRUE;
            entry->timer = 0;
            return entry;
        }
    }

    GF_ASSERT(FALSE);
    return NULL;
}

static void SparkleSpawner_Task(SysTask *task, void *param)
{
    SparkleSpawner *spawner = param;
    SparkleSpawnParams *params = &spawner->params;

    if (params->timer++ >= params->burstInterval) {
        LogoSparkles_SpawnSparkles(spawner->entries, params->sparklesPerBurst, params->type);
        params->timer = 0;

        if (++params->burstsDone >= params->burstCount) {
            SysTask_Done(task);
            spawner->task = NULL;
        }
    }
}

static void SparkleEntry_FallTask(SysTask *task, void *param)
{
    SparkleEntry *entry = param;

    {
        VecFx32 position;

        position = *Sprite_GetPosition(entry->sprite);
        position.y += (FX32_ONE * SPARKLE_FALL_SPEED);

        Sprite_SetPosition(entry->sprite, &position);
    }

    entry->timer++;

    if (entry->timer >= SPARKLE_FALL_FRAMES) {
        SysTask_Done(task);
        entry->task = NULL;
        entry->inUse = FALSE;
        Sprite_SetAnimateFlag(entry->sprite, FALSE);
        Sprite_SetDrawFlag(entry->sprite, FALSE);
    }
}

void TwinklePool_Spawn(TwinklePool *pool, BOOL hidden)
{
    TwinklePool_SpawnTwinkles(pool, 2, hidden);
}

static void TwinklePool_SpawnTwinkles(TwinklePool *pool, const u8 count, BOOL hidden)
{
    u8 i;
    SparkleEntry *entries;
    SparkleEntry *entry;

    entries = pool->entries;

    for (i = 0; i < count; i++) {
        entry = SparkleEntry_Acquire(entries);

        {
            VecFx32 position;

            position = *Sprite_GetPosition(entry->sprite);
            position.x = FX32_ONE * ((16 - 6) + (LCRNG_Next() % 224));
            position.y = FX32_ONE * (192 + (64 - 6) + (LCRNG_Next() % 56));

            Sprite_SetPosition(entry->sprite, &position);
        }

        Sprite_SetAnimateFlag(entry->sprite, TRUE);

        if (hidden == FALSE) {
            Sprite_SetDrawFlag(entry->sprite, TRUE);
        } else {
            Sprite_SetDrawFlag(entry->sprite, FALSE);
        }

        entry->task = SysTask_Start(SparkleEntry_TwinkleTask, entry, SPARKLE_TASK_PRIORITY);
    }
}

static void SparkleEntry_TwinkleTask(SysTask *task, void *param)
{
    SparkleEntry *entry = param;

    entry->timer++;

    if (entry->timer >= TWINKLE_LIFETIME) {
        SysTask_Done(task);
        entry->task = NULL;
        entry->inUse = FALSE;
        Sprite_SetAnimateFlag(entry->sprite, FALSE);
        Sprite_SetDrawFlag(entry->sprite, FALSE);
    }
}
