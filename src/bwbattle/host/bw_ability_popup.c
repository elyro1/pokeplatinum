#include "bwbattle.h"
#include <string.h>
#include "host/bw_ability_popup.h"

#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"

// BW2's ability panels (btlv_scu.c) as screen-space quads. Its BG2/BG3 windows cannot share Platinum's battle BGs.
#define POPUP_WIDTH 144
#define POPUP_HEIGHT 32
#define POPUP_TILES (18 * 4)
#define POPUP_BYTES (POPUP_TILES * 32)
#define POPUP_TEXTURE_BYTES (256 * POPUP_HEIGHT / 2)
#define POPUP_QUEUE 8
#define POPUP_SLIDE_FRAMES 8
#define POPUP_REVEAL_FRAMES 18
#define POPUP_HOLD_FRAMES 54
#define POPUP_HIDE_FRAME (POPUP_SLIDE_FRAMES + POPUP_REVEAL_FRAMES + POPUP_HOLD_FRAMES)
#define POPUP_END_FRAME (POPUP_HIDE_FRAME + POPUP_SLIDE_FRAMES)

typedef struct {
    u16 nickname[32];
    u16 ability[32];
} AbilityPopupText;

static struct {
    u8 art[POPUP_BYTES];
    u8 text[POPUP_BYTES];
    u16 colors[16];
    AbilityPopupText queue[POPUP_QUEUE];
    u8 count;
    u8 frame;
    u8 uploadedColumns;
    BOOL held;
    u32 texture;
    u32 palette;
} sPopups[2];
static BOOL sLoaded;

void BwAbilityPopup_Init(void) {
    MI_CpuClear8(sPopups, sizeof(sPopups));
    sLoaded = FALSE;
}

void BwAbilityPopup_Delete(void) {
    for (int side = 0; side < 2; side++) {
        if (sPopups[side].texture != 0) {
            NNS_GfdFreeTexVram(sPopups[side].texture);
        }
        if (sPopups[side].palette != 0) {
            NNS_GfdFreePlttVram(sPopups[side].palette);
        }
    }
    BwAbilityPopup_Init();
}

static BOOL BwAbilityPopup_Load(void) {
    NNSG2dCharacterData *chars;
    NNSG2dPaletteData *palette;
    void *charFile = GFL_G2DIOReadBGNCGR(11, 0x1e3, FALSE, &chars, HEAPID_BATTLE);
    void *paletteFile = GFL_G2DIOReadNCLR(11, 0x1e4, &palette, HEAPID_BATTLE);
    BOOL loaded = charFile != NULL && paletteFile != NULL && chars->szByte >= POPUP_BYTES * 2
        && palette->szByte >= 16 * 3 * sizeof(u16);

    if (loaded) {
        for (int side = 0; side < 2; side++) {
            MI_CpuCopy8((u8 *)chars->pRawData + side * POPUP_BYTES, sPopups[side].art, POPUP_BYTES);
            MI_CpuCopy16((u16 *)palette->pRawData + (side + 1) * 16, sPopups[side].colors, sizeof(sPopups[side].colors));
            sPopups[side].texture = NNS_GfdAllocTexVram(POPUP_TEXTURE_BYTES, FALSE, 0);
            sPopups[side].palette = NNS_GfdAllocPlttVram(sizeof(sPopups[side].colors), FALSE, 0);
            if (sPopups[side].texture == 0 || sPopups[side].palette == 0) {
                loaded = FALSE;
            }
        }
    }
    if (charFile != NULL) {
        GFL_HeapFree(charFile);
    }
    if (paletteFile != NULL) {
        GFL_HeapFree(paletteFile);
    }
    if (!loaded) {
        BwAbilityPopup_Delete();
    }
    sLoaded = loaded;
    return loaded;
}

static void BwAbilityPopup_CopyText(u16 dest[32], const u16 *source) {
    int i;

    for (i = 0; i < 31 && source[i] != 0xffff; i++) {
        dest[i] = source[i];
    }
    dest[i] = 0xffff;
}

static void BwAbilityPopup_Start(int side) {
    MI_CpuCopy8(sPopups[side].art, sPopups[side].text, POPUP_BYTES);
    BwPlat_DrawText(sPopups[side].queue[0].nickname, sPopups[side].text, 18, 4, 8, -1, 1, 2, 0);
    int x = POPUP_WIDTH - 8 - (int)BwPlat_TextWidth(sPopups[side].queue[0].ability);

    BwPlat_DrawText(sPopups[side].queue[0].ability, sPopups[side].text, 18, 4, x < 4 ? 4 : x, 14, 1, 2, 0);
    sPopups[side].frame = 0;
    sPopups[side].uploadedColumns = 0xff;
    sPopups[side].held = FALSE;
    GFL_SndSEPlay(SEQ_SE_SHOOTER);
}

BOOL BwAbilityPopup_Show(unsigned int side, const u16 *nickname, const u16 *ability) {
    if (side >= 2 || sPopups[side].count == POPUP_QUEUE || (!sLoaded && !BwAbilityPopup_Load())) {
        return FALSE;
    }
    AbilityPopupText *text = &sPopups[side].queue[sPopups[side].count++];

    BwAbilityPopup_CopyText(text->nickname, nickname);
    BwAbilityPopup_CopyText(text->ability, ability);
    if (sPopups[side].count == 1) {
        BwAbilityPopup_Start(side);
    }
    return TRUE;
}

BOOL BwAbilityPopup_IsBusy(void) {
    return sPopups[0].count != 0 || sPopups[1].count != 0;
}

void BwAbilityPopup_SetHeld(unsigned int side, BOOL held) {
    if (side < 2 && sPopups[side].count != 0) {
        sPopups[side].held = held;
    }
}

void BwAbilityPopup_Step(void) {
    for (int side = 0; side < 2; side++) {
        if (sPopups[side].held && sPopups[side].frame == POPUP_HIDE_FRAME) {
            continue;
        }
        if (sPopups[side].count == 0 || ++sPopups[side].frame < POPUP_END_FRAME) {
            continue;
        }
        sPopups[side].count--;
        memmove(&sPopups[side].queue[0], &sPopups[side].queue[1],
            sPopups[side].count * sizeof(AbilityPopupText));
        if (sPopups[side].count != 0) {
            BwAbilityPopup_Start(side);
        }
    }
}

static void BwAbilityPopup_Upload(int side) {
    int columns = sPopups[side].frame - POPUP_SLIDE_FRAMES;
    u8 texels[POPUP_TEXTURE_BYTES];

    columns = columns < 0 ? 0 : columns > 18 ? 18 : columns;
    if (columns == sPopups[side].uploadedColumns) {
        return;
    }
    MI_CpuClear8(texels, sizeof(texels));
    for (int y = 0; y < POPUP_HEIGHT; y++) {
        for (int x = 0; x < POPUP_WIDTH; x += 2) {
            int tile = (y / 8) * 18 + x / 8;
            const u8 *bitmap = x / 8 < columns ? sPopups[side].text : sPopups[side].art;

            texels[y * 128 + x / 2] = bitmap[tile * 32 + (y % 8) * 4 + (x % 8) / 2];
        }
    }
    DC_FlushRange(texels, sizeof(texels));
    GX_BeginLoadTex();
    GX_LoadTex(texels, NNS_GfdGetTexKeyAddr(sPopups[side].texture), sizeof(texels));
    GX_EndLoadTex();
    DC_FlushRange(sPopups[side].colors, sizeof(sPopups[side].colors));
    GX_BeginLoadTexPltt();
    GX_LoadTexPltt(sPopups[side].colors, NNS_GfdGetPlttKeyAddr(sPopups[side].palette), sizeof(sPopups[side].colors));
    GX_EndLoadTexPltt();
    sPopups[side].uploadedColumns = columns;
}

void BwAbilityPopup_Draw(void) {
    if (!BwAbilityPopup_IsBusy()) {
        return;
    }
    int width = GFL_G3DSysGetViewWidth();
    MtxFx44 projection;

    MI_CpuClear8(&projection, sizeof(projection));
    projection._00 = 2 * FX32_ONE / width;
    projection._11 = FX32_ONE / 96;
    projection._32 = -FX32_ONE + 1;
    projection._33 = FX32_ONE;
    NNS_G3dGeFlushBuffer();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_PushMtx();
    G3_LoadMtx44(&projection);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_PushMtx();
    for (int side = 0; side < 2; side++) {
        if (sPopups[side].count == 0) {
            continue;
        }
        BwAbilityPopup_Upload(side);
        int frame = sPopups[side].frame;
        int slide = frame < POPUP_SLIDE_FRAMES ? POPUP_SLIDE_FRAMES - frame
            : frame > POPUP_HIDE_FRAME ? frame - POPUP_HIDE_FRAME : 0;
        int x = side ? width - POPUP_WIDTH + slide * 18 : -slide * 18;
        int y = side ? 44 : 84;

        G3_Identity();
        G3_Translate((x - width / 2) * FX32_ONE, (96 - y) * FX32_ONE, 0);
        G3_Scale(POPUP_WIDTH * FX32_ONE, POPUP_HEIGHT * FX32_ONE, FX32_ONE);
        // Particle effects leave their own texture matrix behind; this overlay uses untransformed pixel coordinates.
        G3_TexImageParam(GX_TEXFMT_PLTT16, GX_TEXGEN_NONE, GX_TEXSIZE_S256, GX_TEXSIZE_T32,
            GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS, NNS_GfdGetTexKeyAddr(sPopups[side].texture));
        G3_TexPlttBase(NNS_GfdGetPlttKeyAddr(sPopups[side].palette), GX_TEXFMT_PLTT16);
        G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 3, 31,
            GX_POLYGON_ATTR_MISC_DEPTHTEST_DECAL);
        G3_Color(GX_RGB(31, 31, 31));
        G3_Begin(GX_BEGIN_QUADS);
        G3_TexCoord(0, 0);
        G3_Vtx(0, 0, 0);
        G3_TexCoord(0, POPUP_HEIGHT * FX32_ONE);
        G3_Vtx(0, -FX16_ONE, 0);
        G3_TexCoord(POPUP_WIDTH * FX32_ONE, POPUP_HEIGHT * FX32_ONE);
        G3_Vtx(FX16_ONE, -FX16_ONE, 0);
        G3_TexCoord(POPUP_WIDTH * FX32_ONE, 0);
        G3_Vtx(FX16_ONE, 0, 0);
        G3_End();
    }
    G3_PopMtx(1);
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_PopMtx(1);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
}
