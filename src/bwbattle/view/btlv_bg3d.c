#include "types.h"
#include "battle/btlv_bg3d.h"
#include "battle/btlv_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "system/palanm.h"

// bwbattle: see btlv_bg3d.h. The 16-color tiles are turned into a 256-color texture whose color index is the tile's
// palette times 16 plus its color, with color 0 of every palette left transparent, so the 256 colors of the main BG
// palettes are its palette

// BG 3's characters and screen, as BW2's BG system would have them in VRAM
#define BG3D_CHARS_SIZE 0x10000
#define BG3D_SCREEN_MAX_TILES (64 * 64)
#define BG3D_PALETTE_SIZE 0x200
// BW2 sets BG 3 to priority 1, the 3D scene's (BG 0); a lower number puts BG 3 in front of it
#define BG3D_SCENE_PRIORITY 1
// The back quad is drawn first at the clear depth, behind all of the scene like the DS's rear plane, and the front one
// last, in front of it. BW2's battle asks for a W-buffer and Platinum's for a Z-buffer in the same frame, so the quads
// are at those depths in both: the back one's W is 4096, the W-buffer's clear depth, and its Z just short of its W,
// the Z-buffer's far plane; the front one's W is 1, the depth of BW2's orthographic sprites, and its Z at the near
// plane
#define BG3D_BACK_W 4096
#define BG3D_FRONT_W 1
// Its own polygon ID, below the particles' (5 and up), so translucency doesn't skip it over translucent polygons
#define BG3D_POLYGON_ID 4
#define BG3D_FRONT_ALPHA 30

static u8 sChars[BG3D_CHARS_SIZE];
static u16 sScreen[BG3D_SCREEN_MAX_TILES];
static u16 sScreenWidth;
static u16 sScreenHeight;
static BOOL sLoaded;
static BOOL sDirty;
static BOOL sEnabled;
static u8 sPriority = BG3D_SCENE_PRIORITY;
static int sOffsetX;
static int sOffsetY;
static int sBlend;
// BW2's wave of BG 3 (btlv_bg.c), while one runs
static struct {
    const s16 *table;
    u32 lines;
    const s16 *shift;
} sWave;

static u8 *sTexels;
static u32 sTexKey;
static u32 sPlttKey;
static u32 sTexSize;

void BtlvBg3d_LoadChars(const void *chars, u32 size, u32 offset) {
    if (offset >= BG3D_CHARS_SIZE) {
        return;
    }
    if (offset + size > BG3D_CHARS_SIZE) {
        size = BG3D_CHARS_SIZE - offset;
    }
    MI_CpuCopy8(chars, sChars + offset, size);
    sDirty = TRUE;
}

void BtlvBg3d_LoadScreen(const NNSG2dScreenData *screen) {
    u32 tiles = (u32)(screen->screenWidth / 8) * (screen->screenHeight / 8);

    if (tiles == 0 || tiles > BG3D_SCREEN_MAX_TILES || screen->szByte < tiles * 2) {
        return;
    }
    sScreenWidth = screen->screenWidth;
    sScreenHeight = screen->screenHeight;
    MI_CpuCopy8(screen->rawData, sScreen, tiles * 2);
    sLoaded = TRUE;
    sDirty = TRUE;
}

void BtlvBg3d_SetEnabled(BOOL enabled) {
    sEnabled = enabled;
}

void BtlvBg3d_SetPriority(u8 priority) {
    sPriority = priority;
}

void BtlvBg3d_SetOffset(int x, int y) {
    sOffsetX = x;
    sOffsetY = y;
}

void BtlvBg3d_SetBlend(int weight) {
    sBlend = weight < 0 ? 0 : weight > 16 ? 16 : weight;
}

void BtlvBg3d_SetWave(const s16 *table, u32 lines, const s16 *shift) {
    sWave.table = table;
    sWave.lines = lines;
    sWave.shift = shift;
}

int BtlvBg3d_GetOffsetX(void) {
    return sOffsetX;
}

int BtlvBg3d_GetOffsetY(void) {
    return sOffsetY;
}

static GXTexSizeS BtlvBg3d_TexSizeS(u32 size) {
    switch (size) {
    case 512:
        return GX_TEXSIZE_S512;
    case 1024:
        return GX_TEXSIZE_S1024;
    default:
        return GX_TEXSIZE_S256;
    }
}

static GXTexSizeT BtlvBg3d_TexSizeT(u32 size) {
    switch (size) {
    case 512:
        return GX_TEXSIZE_T512;
    case 1024:
        return GX_TEXSIZE_T1024;
    default:
        return GX_TEXSIZE_T256;
    }
}

// The screen's tiles as texels, a byte each
static void BtlvBg3d_BuildTexels(void) {
    for (u32 ty = 0; ty < sScreenHeight / 8u; ty++) {
        for (u32 tx = 0; tx < sScreenWidth / 8u; tx++) {
            // A text screen of more than 32 tiles across is in 32 by 32 tile blocks
            u32 block = (tx / 32) + (ty / 32) * (sScreenWidth / 256 ? sScreenWidth / 256 : 1);
            u16 entry = sScreen[block * 1024 + (ty % 32) * 32 + (tx % 32)];
            // 1024 tiles of 32 bytes fill the characters exactly
            const u8 *tile = sChars + (entry & 0x3ff) * 32;
            u8 palette = entry >> 12;

            for (u32 y = 0; y < 8; y++) {
                u32 srcY = (entry & 0x800) ? 7 - y : y;

                for (u32 x = 0; x < 8; x++) {
                    u32 srcX = (entry & 0x400) ? 7 - x : x;
                    u8 pair = tile[srcY * 4 + srcX / 2];
                    u8 color = (srcX & 1) ? pair >> 4 : pair & 0xf;

                    sTexels[(ty * 8 + y) * sScreenWidth + tx * 8 + x] = color == 0 ? 0 : palette * 16 + color;
                }
            }
        }
    }
}

static BOOL BtlvBg3d_IsColumnBlank(u32 x) {
    for (u32 y = 0; y < sScreenHeight; y++) {
        if (sTexels[y * sScreenWidth + x] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

// BW2's backgrounds are a screen's 256 dots, or a little more to shake in, with the rest of the screen's columns blank,
// which the wider view would show. The blank columns carry on the art's edges: each half of them the side of the art
// next to it, in the edge's color where it has one, else the art mirrored
// The texel index of a column's edge color, or -1 when it has none: the art's colors must be nearly all one
// (BG3D_EDGE_SHARE of 16 of the dots that aren't clear), or mostly dark (BG3D_EDGE_DARK_SHARE of 16 of them at most
// BG3D_EDGE_DARK in every channel), the art fading to black like Flare Blitz's, which then carries on in its most
// common dark color. At least BG3D_EDGE_MIN_DOTS of them; the screen is taller than its art, whose rows below are clear
#define BG3D_EDGE_SHARE 15
#define BG3D_EDGE_DARK_SHARE 11
#define BG3D_EDGE_DARK 11
#define BG3D_EDGE_MIN_DOTS 96

static BOOL BtlvBg3d_IsDark(const u16 *colors, u8 index) {
    u16 color = colors[index];

    return (color & 0x1f) <= BG3D_EDGE_DARK && (color >> 5 & 0x1f) <= BG3D_EDGE_DARK
        && (color >> 10 & 0x1f) <= BG3D_EDGE_DARK;
}

static int BtlvBg3d_EdgeColor(u32 x) {
    const u16 *colors = PaletteFade_GetUnfadedBuffer(BtlvEffect_GetPaletteFade(), PALFADE_BUFFER_MAIN_BG);
    u16 counts[256];
    u32 best = 0;
    u32 bestDark = 0;
    u32 dots = 0;
    u32 darkDots = 0;

    MI_CpuClear16(counts, sizeof(counts));
    for (u32 y = 0; y < sScreenHeight; y++) {
        u8 index = sTexels[y * sScreenWidth + x];

        if (index == 0) {
            continue;
        }
        dots++;
        if (++counts[index] > counts[best]) {
            best = index;
        }
        if (colors != NULL && BtlvBg3d_IsDark(colors, index)) {
            darkDots++;
            if (counts[index] > counts[bestDark]) {
                bestDark = index;
            }
        }
    }
    if (dots < BG3D_EDGE_MIN_DOTS) {
        return -1;
    }
    if (counts[best] * 16 >= dots * BG3D_EDGE_SHARE) {
        return (int)best;
    }
    return darkDots * 16 >= dots * BG3D_EDGE_DARK_SHARE ? (int)bestDark : -1;
}

static void BtlvBg3d_MirrorIntoBlank(void) {
    u32 width = sScreenWidth;
    u32 gapStart = 0;
    u32 gapLen = 0;
    u32 artLen;
    int endFill;
    int startFill;

    if (width <= 256) {
        return;
    }
    // The longest run of blank columns, around the screen's edge as the BG wraps
    for (u32 x = 0; x < width; x++) {
        u32 len = 0;

        if (!BtlvBg3d_IsColumnBlank(x) || BtlvBg3d_IsColumnBlank((x + width - 1) % width)) {
            continue;
        }
        while (len < width && BtlvBg3d_IsColumnBlank((x + len) % width)) {
            len++;
        }
        if (len > gapLen) {
            gapStart = x;
            gapLen = len;
        }
    }
    artLen = width - gapLen;
    if (gapLen == 0 || artLen == 0) {
        return;
    }
    // An edge of one color (art that fades out, as Flare Blitz's to black) carries on in it; a patterned one mirrored
    endFill = BtlvBg3d_EdgeColor((gapStart + width - 1) % width);
    startFill = BtlvBg3d_EdgeColor((gapStart + gapLen) % width);
    for (u32 i = 0; i < gapLen; i++) {
        u32 x = (gapStart + i) % width;
        int fill = i < gapLen / 2 ? endFill : startFill;

        if (fill >= 0) {
            for (u32 y = 0; y < sScreenHeight; y++) {
                sTexels[y * width + x] = (u8)fill;
            }
            continue;
        }
        // How far into the art to go back from the edge nearer to it, back and forth if the art is narrower
        u32 far = i < gapLen / 2 ? i : gapLen - 1 - i;
        u32 into = far % (2 * artLen);
        u32 src;

        if (into >= artLen) {
            into = 2 * artLen - 1 - into;
        }
        if (i < gapLen / 2) {
            src = (gapStart + width - 1 - into) % width;
        } else {
            src = (gapStart + gapLen + into) % width;
        }
        for (u32 y = 0; y < sScreenHeight; y++) {
            sTexels[y * width + x] = sTexels[y * width + src];
        }
    }
}

// The texture of the screen's tiles, uploaded again when the tiles or the screen changed
static BOOL BtlvBg3d_Upload(void) {
    u32 size;

    if (!sDirty) {
        return sTexKey != 0;
    }
    sDirty = FALSE;
    if (sPlttKey == 0) {
        sPlttKey = NNS_GfdAllocPlttVram(BG3D_PALETTE_SIZE, FALSE, 0);
        if (sPlttKey == 0) {
            return FALSE;
        }
    }
    if (sTexels == NULL) {
        sTexels = GFL_HeapAllocate(HEAPID_BATTLE, BG3D_SCREEN_MAX_TILES * 64, FALSE, "btlv_bg3d.c", __LINE__);
    }
    BtlvBg3d_BuildTexels();
    BtlvBg3d_MirrorIntoBlank();
    size = (u32)sScreenWidth * sScreenHeight;
    if (sTexKey == 0 || sTexSize != size) {
        if (sTexKey != 0) {
            NNS_GfdFreeTexVram(sTexKey);
        }
        sTexKey = NNS_GfdAllocTexVram(size, FALSE, 0);
        sTexSize = size;
        if (sTexKey == 0) {
            return FALSE;
        }
    }
    DC_FlushRange(sTexels, size);
    GX_BeginLoadTex();
    GX_LoadTex(sTexels, NNS_GfdGetTexKeyAddr(sTexKey), size);
    GX_EndLoadTex();
    return TRUE;
}

static void BtlvBg3d_UploadPalette(void) {
    u16 *colors = PaletteFade_GetFadedBuffer(BtlvEffect_GetPaletteFade(), PALFADE_BUFFER_MAIN_BG);

    if (colors == NULL) {
        return;
    }
    DC_FlushRange(colors, BG3D_PALETTE_SIZE);
    GX_BeginLoadTexPltt();
    GX_LoadTexPltt(colors, NNS_GfdGetPlttKeyAddr(sPlttKey), BG3D_PALETTE_SIZE);
    GX_EndLoadTexPltt();
}

void BtlvBg3d_Draw(BOOL front) {
    s32 width = (s32)GFL_G3DSysGetViewWidth();
    s32 w = front ? BG3D_FRONT_W : BG3D_BACK_W;
    // The back is opaque, as behind the scene BW2's blend only adds the backdrop under it. The front is translucent so
    // that it is drawn last, after the scene's translucent polygons, and more so with BW2's blend, which adds the
    // scene under it, weighted: no script of BW2's sets one, so a mix stands in for it
    int alpha = front ? BG3D_FRONT_ALPHA - sBlend * (BG3D_FRONT_ALPHA - 1) / 16 : 31;
    MtxFx44 projection;
    fx32 s0;
    fx32 t0;

    if (!sEnabled || !sLoaded || (sPriority < BG3D_SCENE_PRIORITY) != front || !BtlvBg3d_Upload()) {
        return;
    }
    BtlvBg3d_UploadPalette();

    // The view's 256 by 192 dots in the middle, as the BG showed them, and as much more as the view is wide
    s0 = (sOffsetX - (width - 256) / 2) * FX32_ONE;
    t0 = sOffsetY * FX32_ONE;

    // An orthographic projection of the dots, all at the same W
    MI_CpuClear8(&projection, sizeof(projection));
    projection._00 = 2 * w * FX32_ONE / width;
    projection._11 = w * FX32_ONE / 96;
    projection._32 = front ? -w * FX32_ONE + 1 : w * FX32_ONE - 1;
    projection._33 = w * FX32_ONE;

    NNS_G3dGeFlushBuffer();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_PushMtx();
    G3_LoadMtx44(&projection);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_PushMtx();
    G3_Identity();
    G3_TexImageParam(GX_TEXFMT_PLTT256, GX_TEXGEN_TEXCOORD, BtlvBg3d_TexSizeS(sScreenWidth),
                     BtlvBg3d_TexSizeT(sScreenHeight), GX_TEXREPEAT_ST, GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_TRNS,
                     NNS_GfdGetTexKeyAddr(sTexKey));
    G3_TexPlttBase(NNS_GfdGetPlttKeyAddr(sPlttKey), GX_TEXFMT_PLTT256);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, BG3D_POLYGON_ID, alpha,
                   GX_POLYGON_ATTR_MISC_DEPTHTEST_DECAL);
    G3_Color(GX_RGB(31, 31, 31));
    // In lines of 1/32 of a unit, so that each of the 192 is a whole number of them
    G3_Translate(-(width / 2) * FX32_ONE, FX32_CONST(96), 0);
    G3_Scale(width * FX32_ONE, 32 * FX32_ONE, FX32_ONE);
    G3_Begin(GX_BEGIN_QUADS);
    // A wave shifts each line by its own offset, as BW2's H-blank task set BG 3's offsets line by line; otherwise
    // one quad covers the lines
    for (u32 line = 0; line < 192; line = sWave.table != NULL ? line + 1 : 192) {
        u32 end = sWave.table != NULL ? line + 1 : 192;
        fx32 s = s0;
        fx32 t = t0 + line * FX32_ONE;

        if (sWave.table != NULL) {
            s16 shift = sWave.table[(line + *sWave.shift + 1) % sWave.lines];

            if (sWave.lines == 192) {
                s += shift * FX32_ONE;
            } else {
                t += shift * FX32_ONE;
            }
        }
        G3_TexCoord(s, t);
        G3_Vtx(0, -(fx16)(line * FX16_ONE / 32), 0);
        G3_TexCoord(s, t + (end - line) * FX32_ONE);
        G3_Vtx(0, -(fx16)(end * FX16_ONE / 32), 0);
        G3_TexCoord(s + width * FX32_ONE, t + (end - line) * FX32_ONE);
        G3_Vtx(FX16_ONE, -(fx16)(end * FX16_ONE / 32), 0);
        G3_TexCoord(s + width * FX32_ONE, t);
        G3_Vtx(FX16_ONE, -(fx16)(line * FX16_ONE / 32), 0);
    }
    G3_End();
    G3_PopMtx(1);
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_PopMtx(1);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
}

void BtlvBg3d_Unload(void) {
    sLoaded = FALSE;
    sPriority = BG3D_SCENE_PRIORITY;
}

void BtlvBg3d_Free(void) {
    if (sTexKey != 0) {
        NNS_GfdFreeTexVram(sTexKey);
        sTexKey = 0;
    }
    if (sPlttKey != 0) {
        NNS_GfdFreePlttVram(sPlttKey);
        sPlttKey = 0;
    }
    if (sTexels != NULL) {
        GFL_HeapFree(sTexels);
        sTexels = NULL;
    }
    sLoaded = FALSE;
    sEnabled = FALSE;
    sDirty = FALSE;
    sPriority = BG3D_SCENE_PRIORITY;
    sOffsetX = 0;
    sOffsetY = 0;
    sBlend = 0;
    sWave.table = NULL;
}
