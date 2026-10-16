#include "game_opening/opening_sprites.h"

#include <nitro.h>
#include <string.h>

#include "constants/graphics.h"

#include "game_opening/opening_timeline.h"
#include "game_opening/opening_sparkles.h"
#include "game_opening/struct_twinkle_pool_decl.h"

#include "cell_transfer.h"
#include "char_transfer.h"
#include "gx_layers.h"
#include "pltt_transfer.h"
#include "render_oam.h"
#include "sprite.h"
#include "sprite_resource.h"
#include "sprite_transfer.h"
#include "sprite_util.h"
#include "system.h"
#include "vram_transfer.h"

#define RESOURCE_SET_COUNT 11
#define HEADER_COUNT       3
#define FLOCK_SIZE         6
#define SUB_SCREEN_OFFSET  (192 << FX32_SHIFT)

// Resource sets, indexed by the SpriteResourcesHeader ID used for each sprite
enum {
    RESOURCE_SET_LEFT_MAIN = 0,
    RESOURCE_SET_LEFT_SUB,
    RESOURCE_SET_RIGHT_MAIN,
    RESOURCE_SET_SLIDE_FROM_LEFT,
    RESOURCE_SET_FLOCK,
    RESOURCE_SET_TWINKLE,
    RESOURCE_SET_SLIDE_FROM_RIGHT,
    RESOURCE_SET_SPINNER_MAIN,
    RESOURCE_SET_SPINNER_SUB,
    RESOURCE_SET_BAND_MAIN,
    RESOURCE_SET_BAND_SUB,
};

typedef struct OpeningSprites_t {
    SpriteList *spriteList;
    G2dRenderer renderer;
    SpriteResourceCollection *collections[MAX_SPRITE_RESOURCE_GEN4];
    SpriteResource *resources[RESOURCE_SET_COUNT][MAX_SPRITE_RESOURCE_GEN4];
    Sprite *sprites[OPENING_SPRITE_COUNT];
    SpriteResourcesHeader headers[HEADER_COUNT];
    NNSG2dCellTransferState *cellTransfer;
} OpeningSprites;

static const u8 sResourceCounts[MAX_SPRITE_RESOURCE_GEN4] = {
    RESOURCE_SET_COUNT,
    RESOURCE_SET_COUNT,
    RESOURCE_SET_COUNT,
    RESOURCE_SET_COUNT
};

static const fx32 sFlockSpeedsA[3] = {
    (FX32_ONE / 2) * 12,
    (FX32_ONE / 2) * 11,
    (FX32_ONE / 2) * 10
};

static const fx32 sFlockSpeedsB[1] = {
    (FX32_ONE / 2) * 10
};

static const fx32 sFlockSpeedsC[2] = {
    (FX32_ONE / 2) * 8,
    (FX32_ONE / 2) * 11
};

static void OpeningSprites_InitTransfers(void)
{
    const int heapID = HEAP_ID_76;

    {
        CharTransferTemplate template = {
            RESOURCE_SET_COUNT, 0x8000, 0x4000, heapID
        };

        CharTransfer_Init(&template);
    }

    PlttTransfer_Init(RESOURCE_SET_COUNT, heapID);
    CharTransfer_ClearBuffers();
    PlttTransfer_Clear();
    VramTransfer_New(3 * 2, heapID);
}

static void OpeningSprites_InitSpriteTemplate(const int resourceID, OpeningSprites *sprites, const int bgPriority, AffineSpriteListTemplate *template, SpriteResourcesHeader *header, int vramTransfer, int vramType)
{
    SpriteResourcesHeader_Init(header, resourceID, resourceID, resourceID, resourceID, 0xffffffff, 0xffffffff, vramTransfer, bgPriority, sprites->collections[0], sprites->collections[1], sprites->collections[2], sprites->collections[3], NULL, NULL);

    template->list = sprites->spriteList;
    template->resourceData = header;
    template->position.x = 0;
    template->position.y = 0;
    template->position.z = 0;
    template->affineScale.x = FX32_ONE;
    template->affineScale.y = FX32_ONE;
    template->affineScale.z = FX32_ONE;
    template->affineZRotation = 0;
    template->priority = 1;
    template->vramType = vramType;
    template->heapID = HEAP_ID_76;
}

int OpeningSprites_GetSize(void)
{
    return sizeof(OpeningSprites);
}

void OpeningSprites_Init(OpeningSprites *sprites)
{
    int i;
    const int heapID = HEAP_ID_76;

    GX_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);
    GXS_SetOBJVRamModeChar(GX_OBJVRAMMODE_CHAR_1D_32K);

    OpeningSprites_InitTransfers();
    NNS_G2dInitOamManagerModule();
    RenderOam_Init(0, 128, 0, 32, 0, 128, 0, 32, heapID);

    sprites->spriteList = SpriteList_InitRendering(OPENING_SPRITE_COUNT + TWINKLE_SPRITE_COUNT, &sprites->renderer, heapID);
    sprites->cellTransfer = CellTransfer_New(3, heapID);

    for (i = 0; i < MAX_SPRITE_RESOURCE_GEN4; i++) {
        sprites->collections[i] = SpriteResourceCollection_New(sResourceCounts[i], i, heapID);
    }

    sprites->resources[0][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 46, FALSE, 0, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[1][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 46, FALSE, 1, NNS_G2D_VRAM_TYPE_2DSUB, heapID);
    sprites->resources[2][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 50, FALSE, 2, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[3][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 87, FALSE, 3, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[4][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 22, FALSE, 4, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[5][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 23, FALSE, 5, NNS_G2D_VRAM_TYPE_2DSUB, heapID);
    sprites->resources[6][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 91, FALSE, 6, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[7][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 83, FALSE, 7, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[8][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 83, FALSE, 8, NNS_G2D_VRAM_TYPE_2DSUB, heapID);
    sprites->resources[9][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 54, FALSE, 9, NNS_G2D_VRAM_TYPE_2DMAIN, heapID);
    sprites->resources[10][SPRITE_RESOURCE_CHAR] = SpriteResourceCollection_AddTiles(sprites->collections[SPRITE_RESOURCE_CHAR], NARC_INDEX_DEMO__TITLE__OP_DEMO, 54, FALSE, 10, NNS_G2D_VRAM_TYPE_2DSUB, heapID);

    sprites->resources[0][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 49, FALSE, 0, NNS_G2D_VRAM_TYPE_2DMAIN, 1, heapID);
    sprites->resources[1][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 49, FALSE, 1, NNS_G2D_VRAM_TYPE_2DSUB, 1, heapID);
    sprites->resources[2][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 53, FALSE, 2, NNS_G2D_VRAM_TYPE_2DMAIN, 1, heapID);
    sprites->resources[3][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 90, FALSE, 3, NNS_G2D_VRAM_TYPE_2DMAIN, 1, heapID);
    sprites->resources[4][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 20, FALSE, 4, NNS_G2D_VRAM_TYPE_2DMAIN, 1, heapID);
    sprites->resources[5][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 21, FALSE, 5, NNS_G2D_VRAM_TYPE_2DSUB, 1, heapID);
    sprites->resources[6][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 94, FALSE, 6, NNS_G2D_VRAM_TYPE_2DMAIN, 1, heapID);
    sprites->resources[7][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 86, FALSE, 7, NNS_G2D_VRAM_TYPE_2DMAIN, 1, heapID);
    sprites->resources[8][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 86, FALSE, 8, NNS_G2D_VRAM_TYPE_2DSUB, 1, heapID);
    sprites->resources[9][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 57, FALSE, 9, NNS_G2D_VRAM_TYPE_2DMAIN, 3, heapID);
    sprites->resources[10][SPRITE_RESOURCE_PLTT] = SpriteResourceCollection_AddPalette(sprites->collections[SPRITE_RESOURCE_PLTT], NARC_INDEX_DEMO__TITLE__OP_DEMO, 57, FALSE, 10, NNS_G2D_VRAM_TYPE_2DSUB, 3, heapID);

    sprites->resources[0][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 48, FALSE, 0, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[1][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 48, FALSE, 1, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[2][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 51, FALSE, 2, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[3][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 89, FALSE, 3, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[4][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 24, FALSE, 4, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[6][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 93, FALSE, 6, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[7][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 85, FALSE, 7, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[8][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 85, FALSE, 8, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[9][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 55, FALSE, 9, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[10][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 55, FALSE, 10, SPRITE_RESOURCE_CELL, heapID);
    sprites->resources[5][SPRITE_RESOURCE_CELL] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_CELL], NARC_INDEX_DEMO__TITLE__OP_DEMO, 25, FALSE, 5, SPRITE_RESOURCE_CELL, heapID);

    sprites->resources[0][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 47, FALSE, 0, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[1][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 47, FALSE, 1, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[2][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 52, FALSE, 2, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[3][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 88, FALSE, 3, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[4][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 26, FALSE, 4, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[5][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 27, FALSE, 5, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[6][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 92, FALSE, 6, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[7][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 84, FALSE, 7, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[8][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 84, FALSE, 8, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[9][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 56, FALSE, 9, SPRITE_RESOURCE_ANIM, heapID);
    sprites->resources[10][SPRITE_RESOURCE_ANIM] = SpriteResourceCollection_Add(sprites->collections[SPRITE_RESOURCE_ANIM], NARC_INDEX_DEMO__TITLE__OP_DEMO, 56, FALSE, 10, SPRITE_RESOURCE_ANIM, heapID);

    for (i = 0; i < RESOURCE_SET_COUNT; i++) {
        SpriteTransfer_RequestCharAtEnd(sprites->resources[i][SPRITE_RESOURCE_CHAR]);
        SpriteTransfer_RequestPlttFreeSpace(sprites->resources[i][SPRITE_RESOURCE_PLTT]);
    }

    GXLayers_EngineAToggleLayers(GX_PLANEMASK_OBJ, 1);
}

void OpeningSprites_Free(OpeningSprites *sprites)
{
    u8 i;

    for (i = 0; i < RESOURCE_SET_COUNT; i++) {
        SpriteTransfer_ResetCharTransfer(sprites->resources[i][SPRITE_RESOURCE_CHAR]);
        SpriteTransfer_ResetPlttTransfer(sprites->resources[i][SPRITE_RESOURCE_PLTT]);
    }

    for (i = 0; i < MAX_SPRITE_RESOURCE_GEN4; i++) {
        SpriteResourceCollection_Delete(sprites->collections[i]);
    }

    SpriteList_Delete(sprites->spriteList);

    for (i = 0; i < HEADER_COUNT; i++) {
        SpriteResourcesHeader_Clear(&sprites->headers[i]);
    }

    if (sprites->cellTransfer != NULL) {
        CellTransfer_Free(sprites->cellTransfer);
        sprites->cellTransfer = NULL;
    }

    RenderOam_Free();
    CharTransfer_Free();
    PlttTransfer_Free();
    VramTransfer_Free();
}

static Sprite *OpeningSprites_AddSprite(OpeningSprites *sprites, enum OpeningSpriteSlot slot, AffineSpriteListTemplate *template, BOOL animating)
{
    sprites->sprites[slot] = SpriteList_AddAffine(template);

    Sprite_SetAnimateFlag(sprites->sprites[slot], animating);
    Sprite_SetDrawFlag(sprites->sprites[slot], FALSE);

    return sprites->sprites[slot];
}

void OpeningSprites_CreateSprites(OpeningSprites *sprites)
{
    int i;
    SpriteResourcesHeader header;
    AffineSpriteListTemplate template;
    static const int flockSlots[FLOCK_SIZE] = {
        OPENING_SPRITE_FLOCK_1,
        OPENING_SPRITE_FLOCK_2,
        OPENING_SPRITE_FLOCK_3,
        OPENING_SPRITE_FLOCK_4,
        OPENING_SPRITE_FLOCK_5,
        OPENING_SPRITE_FLOCK_6,
    };
    static const int flockStartX[FLOCK_SIZE] = {
        (256 + 8),
        256 + 32,
        256 + 32,
        256 + 32,
        256 + 32,
        (256 + 32)
    };
    static const int flockStartY[FLOCK_SIZE] = {
        (16 * 8),
        14 * 8,
        18 * 8,
        20 * 8,
        18 * 8,
        (22 * 8)
    };

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_LEFT_MAIN, sprites, 0, &template, &sprites->headers[0], 1, NNS_G2D_VRAM_TYPE_2DMAIN);
    template.position.x = FX32_ONE * (128 - 48);
    template.position.y = FX32_ONE * (14 * 8);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_LEFT_MAIN, &template, FALSE);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_LEFT_SUB, sprites, 0, &template, &sprites->headers[2], 1, NNS_G2D_VRAM_TYPE_2DSUB);
    template.position.x = FX32_ONE * (128 - 48);
    template.position.y = FX32_ONE * (14 * 8) + SUB_SCREEN_OFFSET;
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_LEFT_SUB, &template, FALSE);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_RIGHT_MAIN, sprites, 0, &template, &sprites->headers[1], 1, NNS_G2D_VRAM_TYPE_2DMAIN);
    template.position.x = FX32_ONE * (128 + 48);
    template.position.y = FX32_ONE * (14 * 8);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_RIGHT_MAIN, &template, FALSE);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_SLIDE_FROM_LEFT, sprites, 0, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
    template.position.x = FX32_ONE * -64;
    template.position.y = FX32_ONE * (14 * 8);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_SLIDE_FROM_LEFT, &template, TRUE);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_SLIDE_FROM_RIGHT, sprites, 0, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
    template.position.x = FX32_ONE * (256 + 64);
    template.position.y = FX32_ONE * (14 * 8);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_SLIDE_FROM_RIGHT, &template, TRUE);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_SPINNER_MAIN, sprites, 1, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
    template.position.x = FX32_ONE * 128;
    template.position.y = FX32_ONE * (12 * 8);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_SPINNER_MAIN, &template, TRUE);
    Sprite_SetAffineOverwriteMode(sprites->sprites[OPENING_SPRITE_SPINNER_MAIN], AFFINE_OVERWRITE_MODE_NORMAL);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_SPINNER_SUB, sprites, 1, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DSUB);
    template.position.x = FX32_ONE * 128;
    template.position.y = FX32_ONE * (12 * 8) + SUB_SCREEN_OFFSET;
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_SPINNER_SUB, &template, TRUE);
    Sprite_SetAffineOverwriteMode(sprites->sprites[OPENING_SPRITE_SPINNER_SUB], AFFINE_OVERWRITE_MODE_NORMAL);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_BAND_MAIN, sprites, 3, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
    template.position.x = FX32_ONE * 128;
    template.position.y = FX32_ONE * (13 * 8);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_BAND_MAIN_A, &template, TRUE);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_BAND_MAIN_B, &template, TRUE);

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_BAND_SUB, sprites, 3, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DSUB);
    template.position.x = FX32_ONE * 128;
    template.position.y = FX32_ONE * (13 * 8) + SUB_SCREEN_OFFSET;
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_BAND_SUB_A, &template, TRUE);
    OpeningSprites_AddSprite(sprites, OPENING_SPRITE_BAND_SUB_B, &template, TRUE);

    for (i = 0; i < FLOCK_SIZE; i++) {
        OpeningSprites_InitSpriteTemplate(RESOURCE_SET_FLOCK, sprites, 0, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DMAIN);
        template.position.x = FX32_ONE * flockStartX[i];
        template.position.y = FX32_ONE * flockStartY[i];
        OpeningSprites_AddSprite(sprites, flockSlots[i], &template, TRUE);
    }
}

void OpeningSprites_CreateTwinkleSprites(OpeningSprites *sprites, TwinklePool *twinkles)
{
    int i;
    SpriteResourcesHeader header;
    AffineSpriteListTemplate template;
    Sprite **slot;

    OpeningSprites_InitSpriteTemplate(RESOURCE_SET_TWINKLE, sprites, 0, &template, &header, 0, NNS_G2D_VRAM_TYPE_2DSUB);

    for (i = 0; i < TWINKLE_SPRITE_COUNT; i++) {
        slot = TwinklePool_GetSpriteSlot(twinkles, i);
        (*slot) = SpriteList_AddAffine(&template);
        Sprite_SetAnimateFlag(*slot, FALSE);
        Sprite_SetDrawFlag(*slot, FALSE);
    }
}

void OpeningSprites_HideTwinkleSprites(TwinklePool *twinkles)
{
    int i;
    Sprite **slot;

    for (i = 0; i < TWINKLE_SPRITE_COUNT; i++) {
        slot = TwinklePool_GetSpriteSlot(twinkles, i);
        Sprite_SetDrawFlag(*slot, FALSE);
    }
}

void OpeningSprites_Update(OpeningSprites *sprites)
{
    SpriteList_Update(sprites->spriteList);

    if (sprites->cellTransfer != NULL) {
        CellTransfer_Update();
    }
}

void OpeningSprites_SetVisible(OpeningSprites *sprites, const u8 slot, const u8 visible)
{
    Sprite_SetDrawFlag(sprites->sprites[slot], visible);
}

void OpeningSprites_SetLeftRightAnimating(OpeningSprites *sprites, const u8 animating)
{
    Sprite_SetAnimateFlag(sprites->sprites[OPENING_SPRITE_LEFT_MAIN], animating);
    Sprite_SetAnimateFlag(sprites->sprites[OPENING_SPRITE_RIGHT_MAIN], animating);
    Sprite_SetAnimateFlag(sprites->sprites[OPENING_SPRITE_LEFT_SUB], animating);
}

void OpeningSprites_SpreadLeftRight(OpeningSprites *sprites)
{
    VecFx32 position;

    position = *Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_LEFT_SUB]);
    position.x -= 0x200;

    Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_LEFT_SUB], &position);

    position = *Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_RIGHT_MAIN]);
    position.x += 0x200;

    Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_RIGHT_MAIN], &position);
}

BOOL OpeningSprites_UpdateSlideFromLeft(OpeningSprites *sprites, int frame)
{
    VecFx32 position;

    if (frame < LANDSCAPE_FRAME_SLIDE_IN_1) {
        return FALSE;
    }

    if (frame == LANDSCAPE_FRAME_SLIDE_IN_1) {
        OpeningSprites_SetVisible(sprites, OPENING_SPRITE_SLIDE_FROM_LEFT, TRUE);
    }

    if (frame < LANDSCAPE_FRAME_SLIDE_OUT) {
        position = *Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_LEFT]);
        position.x += (FX32_ONE * 18);

        if (position.x >= (FX32_ONE * (128 + 48))) {
            position.x = (FX32_ONE * (128 + 48));
            Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_LEFT], &position);
            return TRUE;
        }

        Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_LEFT], &position);
        return FALSE;
    } else {
        position = *Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_LEFT]);
        position.x += (FX32_ONE * 18);

        if (position.x >= (256 + 64) * FX32_ONE) {
            position.x = (256 + 64) * FX32_ONE;
            Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_LEFT], &position);
            OpeningSprites_SetVisible(sprites, OPENING_SPRITE_SLIDE_FROM_LEFT, FALSE);
            return TRUE;
        }

        Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_LEFT], &position);
        return FALSE;
    }
}

BOOL OpeningSprites_UpdateSlideFromRight(OpeningSprites *sprites, int frame)
{
    VecFx32 position;

    if (frame < LANDSCAPE_FRAME_SLIDE_IN_2) {
        return FALSE;
    }

    if (frame == LANDSCAPE_FRAME_SLIDE_IN_2) {
        OpeningSprites_SetVisible(sprites, OPENING_SPRITE_SLIDE_FROM_RIGHT, TRUE);
    }

    if (frame < LANDSCAPE_FRAME_SLIDE_OUT) {
        position = *Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_RIGHT]);
        position.x += (-(FX32_ONE * 18));

        if (position.x <= (FX32_ONE * (128 - 48))) {
            position.x = (FX32_ONE * (128 - 48));
            Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_RIGHT], &position);
            return TRUE;
        }

        Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_RIGHT], &position);
        return FALSE;
    } else {
        position = *Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_RIGHT]);
        position.x += (-(FX32_ONE * 18));

        if (position.x <= -64 * FX32_ONE) {
            position.x = -64 * FX32_ONE;
            Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_RIGHT], &position);
            OpeningSprites_SetVisible(sprites, OPENING_SPRITE_SLIDE_FROM_RIGHT, FALSE);
            return TRUE;
        }

        Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SLIDE_FROM_RIGHT], &position);
        return FALSE;
    }
}

void OpeningSprites_ScrollBands(OpeningSprites *sprites, fx32 speed)
{
    const VecFx32 *current;
    VecFx32 position;
    int rightwardBand, leftwardBand;

    if (gSystem.whichScreenIs3D == DS_SCREEN_MAIN) {
        rightwardBand = OPENING_SPRITE_BAND_MAIN_A;
        leftwardBand = OPENING_SPRITE_BAND_SUB_A;
    } else {
        rightwardBand = OPENING_SPRITE_BAND_SUB_A;
        leftwardBand = OPENING_SPRITE_BAND_MAIN_A;
    }

    current = Sprite_GetPosition(sprites->sprites[rightwardBand]);
    position = *current;
    position.x += speed;

    if (position.x >= (128 + 256) * FX32_ONE) {
        position.x -= 256 * FX32_ONE;
    }

    Sprite_SetPosition(sprites->sprites[rightwardBand], &position);
    position.x -= 256 * FX32_ONE;
    Sprite_SetPosition(sprites->sprites[rightwardBand + 1], &position);

    current = Sprite_GetPosition(sprites->sprites[leftwardBand]);
    position = *current;
    position.x -= speed;

    if (position.x <= (128 - 256) * FX32_ONE) {
        position.x += 256 * FX32_ONE;
    }

    Sprite_SetPosition(sprites->sprites[leftwardBand], &position);
    position.x += 256 * FX32_ONE;
    Sprite_SetPosition(sprites->sprites[leftwardBand + 1], &position);
}

void OpeningSprites_SetBandPalettes(OpeningSprites *sprites, BOOL useFinalPalette)
{
    const VecFx32 *current;
    VecFx32 position;

    if (!useFinalPalette) {
        if (gSystem.whichScreenIs3D == DS_SCREEN_MAIN) {
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_MAIN_A], 0);
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_MAIN_B], 0);
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_SUB_A], 1);
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_SUB_B], 1);
        } else {
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_MAIN_A], 1);
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_MAIN_B], 1);
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_SUB_A], 0);
            Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_SUB_B], 0);
        }
    } else {
        Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_MAIN_A], 2);
        Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_MAIN_B], 2);
        Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_SUB_A], 2);
        Sprite_SetExplicitPaletteOffsetAutoAdjust(sprites->sprites[OPENING_SPRITE_BAND_SUB_B], 2);
    }

    current = Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_BAND_MAIN_A]);
    position = *current;
    position.x = 128 * FX32_ONE;

    Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_BAND_MAIN_A], &position);

    current = Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_BAND_SUB_A]);
    position = *current;
    position.x = 128 * FX32_ONE;

    Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_BAND_SUB_A], &position);
}

void OpeningSprites_UpdateSpinners(OpeningSprites *sprites, int frame)
{
    u16 rotation;
    const VecFx32 *current;
    VecFx32 position;
    s32 extraSpin = frame - SCROLL_FRAME_SPIN_START;

    if (extraSpin < 0) {
        extraSpin = 0;
    }

    rotation = Sprite_GetAffineZRotation(sprites->sprites[OPENING_SPRITE_SPINNER_MAIN]);
    Sprite_SetAffineZRotation(sprites->sprites[OPENING_SPRITE_SPINNER_MAIN], rotation + 0x1c00 + extraSpin);

    rotation = Sprite_GetAffineZRotation(sprites->sprites[OPENING_SPRITE_SPINNER_SUB]);
    Sprite_SetAffineZRotation(sprites->sprites[OPENING_SPRITE_SPINNER_SUB], rotation - 0x1c00 - extraSpin);

    if (frame >= SCROLL_FRAME_SPIN_START) {
        current = Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_SPINNER_MAIN]);
        position = *current;
        position.x += 0x1000;

        Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SPINNER_MAIN], &position);

        current = Sprite_GetPosition(sprites->sprites[OPENING_SPRITE_SPINNER_SUB]);
        position = *current;
        position.x -= 0x1000;

        Sprite_SetPosition(sprites->sprites[OPENING_SPRITE_SPINNER_SUB], &position);
    }
}

static void OpeningSprites_MoveFlockGroup(OpeningSprites *sprites, const int firstSlot, const int count, const fx32 *speeds, const fx32 riseSpeed)
{
    u8 i;
    VecFx32 position;

    for (i = 0; i < count; i++) {
        position = *Sprite_GetPosition(sprites->sprites[firstSlot + i]);
        position.x -= speeds[i];
        position.y -= riseSpeed;
        Sprite_SetPosition(sprites->sprites[firstSlot + i], &position);
    }
}

void OpeningSprites_UpdateFlock(OpeningSprites *sprites, const int frame)
{
    if (frame >= LANDSCAPE_FRAME_FLOCK_1) {
        OpeningSprites_MoveFlockGroup(sprites, OPENING_SPRITE_FLOCK_1, 3, sFlockSpeedsA, FX32_ONE * 4);
    }

    if (frame >= LANDSCAPE_FRAME_FLOCK_2) {
        OpeningSprites_MoveFlockGroup(sprites, OPENING_SPRITE_FLOCK_4, 1, sFlockSpeedsB, FX32_ONE * 3);
    }

    if (frame >= LANDSCAPE_FRAME_FLOCK_3) {
        OpeningSprites_MoveFlockGroup(sprites, OPENING_SPRITE_FLOCK_5, 2, sFlockSpeedsC, FX32_ONE * 3);
    }
}
