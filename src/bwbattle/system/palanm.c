#include "types.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "system/palanm.h"

// Palette fades, and blends and filters of colors

// A color's channels
typedef struct {
    u16 r : 5;
    u16 g : 5;
    u16 b : 5;
    u16 unused : 1;
} PaletteColor;

// How a buffer fades: the palettes it fades, and its steps from cur to end of 16 toward color
typedef struct {
    u16 paletteMask;
    u16 delay : 6;
    u16 cur : 5;
    u16 end : 5;
    u16 color : 15;
    u16 direction : 1;
    u16 step : 5;
    u16 delayCounter : 6;
} FadeControl;

typedef struct {
    u16 *unfaded;
    u16 *faded;
    u32 size;
    FadeControl fade;
} PaletteBuffer;

// A channel blended fraction of 16 of the way from one value to another
#define BLEND_CHANNEL(from, to, fraction) ((from) + ((((to) - (from)) * (fraction)) >> 4))

// The fade task's states
#define PALFADE_STATE_IDLE 0
#define PALFADE_STATE_ACTIVE 1

struct PaletteFade {
    PaletteBuffer buffers[PALFADE_BUFFER_COUNT];
    u16 state : 2;
    // The buffers still fading
    u16 activeMask : 14;
    // The buffers PaletteFade_Transfer copies
    u16 transferMask : 14;
    u16 taskActive : 1;
    u16 transferAll : 1;
    u8 stopRequested;
};

static void PaletteFade_SetBuffer(PaletteFade *fade, u32 buffer, u16 *unfaded, u16 *faded, u32 size);
static u8 IsBitSet(u16 mask, u16 bit);
static void PaletteFade_SetTransfer(PaletteFade *fade, u8 buffer);
static void MaskPalettes(int buffer, PaletteBuffer *paletteBuffer, u16 *paletteMask);
static void FadeControl_Init(FadeControl *control, u16 paletteMask, s8 delay, u8 start, u8 end, u16 color);
static void PaletteFade_Task(TCB *tcb, void *data);
static void PaletteFade_StepStd(PaletteFade *fade);
static void PaletteFade_StepExt(PaletteFade *fade);
static void PaletteFade_StepBuffer(PaletteFade *fade, u8 buffer, u16 paletteSize);
static void PaletteFade_ApplyBuffer(PaletteFade *fade, u16 buffer, u16 paletteSize);
static void BlendFadeColors(const u16 *src, u16 *dst, FadeControl *control, u32 count);
static void FadeControl_Advance(PaletteFade *fade, u8 buffer, FadeControl *control);

// The weights ColorFilter_ApplyVintage gives each channel of a gray, out of 256
static u32 g_FieldColorPostFXWeightR = 155;
static u32 g_FieldColorPostFXWeightG = 135;
static u32 g_FieldColorPostFXWeightB = 85;

PaletteFade *PaletteFade_Create(u32 heapId) {
    PaletteFade *fade = GFL_HeapAllocate(heapId, sizeof(PaletteFade), FALSE, "palanm.c", 89);

    MI_CpuFill8(fade, 0, sizeof(PaletteFade));
    return fade;
}

void PaletteFade_Free(PaletteFade *fade) {
    GFL_HeapFree(fade);
}

static void PaletteFade_SetBuffer(PaletteFade *fade, u32 buffer, u16 *unfaded, u16 *faded, u32 size) {
    fade->buffers[buffer].unfaded = unfaded;
    fade->buffers[buffer].faded = faded;
    fade->buffers[buffer].size = size;
}

void PaletteFade_AllocBuffer(PaletteFade *fade, u32 buffer, u32 size, HeapID heapId) {
    u16 *unfaded = GFL_HeapAllocate(heapId, size, FALSE, "palanm.c", 143);
    u16 *faded = GFL_HeapAllocate(heapId, size, FALSE, "palanm.c", 144);

    PaletteFade_SetBuffer(fade, buffer, unfaded, faded, size);
}

void PaletteFade_FreeBuffer(PaletteFade *fade, u32 buffer) {
    GFL_HeapFree(fade->buffers[buffer].unfaded);
    GFL_HeapFree(fade->buffers[buffer].faded);
}

void PaletteFade_LoadData(PaletteFade *fade, const void *src, u32 buffer, u16 offset, u16 size) {
    MIi_CpuCopy16(src, fade->buffers[buffer].unfaded + offset, size);
    MIi_CpuCopy16(src, fade->buffers[buffer].faded + offset, size);
}

void PaletteFade_LoadNCLREx(PaletteFade *fade, u32 arcId, u32 fileId, HeapID heapId, u32 buffer, u32 size, u16 offset,
                            u16 srcOffset) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(arcId, fileId, &palette, HEAPID_TAIL(heapId));

    if (size == 0) {
        size = palette->szByte;
    }
    PaletteFade_LoadData(fade, (u16 *)palette->pRawData + srcOffset, buffer, offset, size);
    GFL_HeapFree(file);
}

void PaletteFade_LoadNCLR(PaletteFade *fade, u32 arcId, u32 fileId, HeapID heapId, u32 buffer, u32 size, u16 offset) {
    PaletteFade_LoadNCLREx(fade, arcId, fileId, heapId, buffer, size, offset, 0);
}

void PaletteFade_LoadArcNCLREx(PaletteFade *fade, ArcTool *arc, u32 fileId, HeapID heapId, u32 buffer, u32 size,
                               u16 offset, u16 srcOffset) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLRArc(arc, fileId, &palette, HEAPID_TAIL(heapId));

    if (size == 0) {
        size = palette->szByte;
    }
    PaletteFade_LoadData(fade, (u16 *)palette->pRawData + srcOffset, buffer, offset, size);
    GFL_HeapFree(file);
}

void PaletteFade_LoadArcNCLR(PaletteFade *fade, ArcTool *arc, u32 fileId, HeapID heapId, u32 buffer, u32 size,
                             u16 offset) {
    PaletteFade_LoadArcNCLREx(fade, arc, fileId, heapId, buffer, size, offset, 0);
}

void PaletteFade_LoadFromVRAM(PaletteFade *fade, u32 vram, u16 offset, u32 size) {
    u16 *src;

    switch (vram) {
    case PALFADE_VRAM_MAIN_BG:
        src = (u16 *)HW_BG_PLTT;
        break;
    case PALFADE_VRAM_SUB_BG:
        src = (u16 *)HW_DB_BG_PLTT;
        break;
    case PALFADE_VRAM_MAIN_OBJ:
#ifdef BWBATTLE
        // bwbattle: the view's OBJ palettes are the OBJ extension's (BW_OBJ_EXT_PLTT_OFFSET)
        src = (u16 *)HW_OBJ_PLTT_EXT;
#else
        src = (u16 *)HW_OBJ_PLTT;
#endif
        break;
    case PALFADE_VRAM_SUB_OBJ:
        src = (u16 *)HW_DB_OBJ_PLTT;
        break;
    default:
        return;
    }
    PaletteFade_LoadData(fade, src + offset, vram, offset, size);
}

u16 *PaletteFade_GetUnfadedBuffer(PaletteFade *fade, u32 buffer) {
    return fade->buffers[buffer].unfaded;
}

u16 *PaletteFade_GetFadedBuffer(PaletteFade *fade, u32 buffer) {
    return fade->buffers[buffer].faded;
}

u8 PaletteFade_StartFade(PaletteFade *fade, u16 bufferMask, u16 paletteMask, s8 delay, u8 start, u8 end, u16 color,
                         TCBManager *tcbManager) {
    u16 allPalettes = paletteMask;
    u8 started = FALSE;
    u8 i;

    for (i = 0; i < PALFADE_BUFFER_COUNT; i++) {
        if (IsBitSet(bufferMask, i) == TRUE && IsBitSet(fade->activeMask, i) == FALSE) {
            MaskPalettes(i, &fade->buffers[i], &paletteMask);
            FadeControl_Init(&fade->buffers[i].fade, paletteMask, delay, start, end, color);
            PaletteFade_SetTransfer(fade, i);
            if (i >= PALFADE_BUFFERS_STD) {
                PaletteFade_ApplyBuffer(fade, i, 256);
            } else {
                PaletteFade_ApplyBuffer(fade, i, 16);
            }
            paletteMask = allPalettes;
            started = TRUE;
        }
    }
    if (started == TRUE) {
        fade->activeMask |= bufferMask;
        if (fade->taskActive == FALSE) {
            fade->taskActive = TRUE;
            fade->state = PALFADE_STATE_ACTIVE;
            fade->stopRequested = FALSE;
            GFL_TCBMgrAddTask(tcbManager, PaletteFade_Task, fade, -2);
        }
    }
    return started;
}

u8 PaletteFade_RestartFade(PaletteFade *fade, u16 bufferMask, u16 paletteMask, s8 delay, u8 start, u8 end, u16 color,
                           TCBManager *tcbManager) {
    u16 allPalettes = paletteMask;
    u8 started = FALSE;
    u8 i;

    for (i = 0; i < PALFADE_BUFFER_COUNT; i++) {
        if (IsBitSet(bufferMask, i) == TRUE) {
            MaskPalettes(i, &fade->buffers[i], &paletteMask);
            FadeControl_Init(&fade->buffers[i].fade, paletteMask, delay, start, end, color);
            PaletteFade_SetTransfer(fade, i);
            if (i >= PALFADE_BUFFERS_STD) {
                PaletteFade_ApplyBuffer(fade, i, 256);
            } else {
                PaletteFade_ApplyBuffer(fade, i, 16);
            }
            paletteMask = allPalettes;
            started = TRUE;
        }
    }
    if (started == TRUE) {
        fade->activeMask = bufferMask;
        if (fade->taskActive == FALSE) {
            fade->taskActive = TRUE;
            fade->state = PALFADE_STATE_ACTIVE;
            fade->stopRequested = FALSE;
            GFL_TCBMgrAddTask(tcbManager, PaletteFade_Task, fade, -2);
        }
    }
    return started;
}

static u8 IsBitSet(u16 mask, u16 bit) {
    BOOL result = TRUE;

    if ((mask & (1 << bit)) == 0) {
        result = FALSE;
    }
    return result;
}

static void PaletteFade_SetTransfer(PaletteFade *fade, u8 buffer) {
    if (IsBitSet(fade->transferMask, buffer) != TRUE) {
        fade->transferMask |= (u16)(1 << buffer);
    }
}

// Keeps the palettes of paletteMask that the buffer has
static void MaskPalettes(int buffer, PaletteBuffer *paletteBuffer, u16 *paletteMask) {
    u8 count;
    u16 mask;
    u8 i;

    if (buffer < PALFADE_BUFFERS_STD) {
        count = paletteBuffer->size / 32;
    } else {
        count = paletteBuffer->size / 512;
    }
    mask = 0;
    for (i = 0; i < count; i++) {
        mask += (u16)(1 << i);
    }
    *paletteMask &= mask;
}

static void FadeControl_Init(FadeControl *control, u16 paletteMask, s8 delay, u8 start, u8 end, u16 color) {
    if (delay < 0) {
        u8 step = MATH_ABS(delay) + 2;

        if (step > 16) {
            step = 16;
        }
        control->step = step;
        control->delay = 0;
    } else {
        control->step = 2;
        control->delay = delay;
    }
    control->paletteMask = paletteMask;
    control->cur = start;
    control->end = end;
    control->color = color;
    control->delayCounter = control->delay;
    if (start < end) {
        control->direction = 0;
    } else {
        control->direction = 1;
    }
}

static void PaletteFade_Task(TCB *tcb, void *data) {
    PaletteFade *fade = data;

    if (fade->stopRequested == TRUE) {
        fade->stopRequested = FALSE;
        fade->transferMask = 0;
        fade->activeMask = 0;
        fade->taskActive = FALSE;
        GFL_TCBRemove(tcb);
        return;
    }
    if (fade->state == PALFADE_STATE_ACTIVE) {
        fade->transferMask = fade->activeMask;
        PaletteFade_StepStd(fade);
        PaletteFade_StepExt(fade);
        if (fade->activeMask == 0) {
            fade->taskActive = FALSE;
            GFL_TCBRemove(tcb);
        }
    }
}

void PaletteFade_RequestStop(PaletteFade *fade) {
    if (fade->activeMask != 0) {
        fade->stopRequested = TRUE;
    }
}

static void PaletteFade_StepStd(PaletteFade *fade) {
    u8 i;

    for (i = 0; i < PALFADE_BUFFERS_STD; i++) {
        PaletteFade_StepBuffer(fade, i, 16);
    }
}

static void PaletteFade_StepExt(PaletteFade *fade) {
    u8 i;

    for (i = PALFADE_BUFFERS_STD; i < PALFADE_BUFFER_COUNT; i++) {
        PaletteFade_StepBuffer(fade, i, 256);
    }
}

static void PaletteFade_StepBuffer(PaletteFade *fade, u8 buffer, u16 paletteSize) {
    if (IsBitSet(fade->activeMask, buffer)) {
        PaletteBuffer *paletteBuffer = &fade->buffers[buffer];

        if (paletteBuffer->fade.delayCounter < paletteBuffer->fade.delay) {
            paletteBuffer->fade.delayCounter++;
        } else {
            paletteBuffer->fade.delayCounter = 0;
            PaletteFade_ApplyBuffer(fade, buffer, paletteSize);
        }
    }
}

static void PaletteFade_ApplyBuffer(PaletteFade *fade, u16 buffer, u16 paletteSize) {
    u32 i;
    PaletteBuffer *paletteBuffer = &fade->buffers[buffer];

    for (i = 0; i < 16; i++) {
        if (IsBitSet(paletteBuffer->fade.paletteMask, i)) {
            BlendFadeColors(paletteBuffer->unfaded + i * paletteSize, paletteBuffer->faded + i * paletteSize,
                            &paletteBuffer->fade, paletteSize);
        }
    }
    FadeControl_Advance(fade, buffer, &paletteBuffer->fade);
}

static void BlendFadeColors(const u16 *src, u16 *dst, FadeControl *control, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        u8 r = BLEND_CHANNEL(src[i] & 0x1f, control->color & 0x1f, control->cur);
        u8 b = BLEND_CHANNEL((src[i] >> 10) & 0x1f, (control->color >> 10) & 0x1f, control->cur);
        u8 g = BLEND_CHANNEL((src[i] >> 5) & 0x1f, (control->color >> 5) & 0x1f, control->cur);

        dst[i] = (b << 10) | (g << 5) | r;
    }
}

static void FadeControl_Advance(PaletteFade *fade, u8 buffer, FadeControl *control) {
    s16 cur;

    if (control->cur == control->end) {
        if (fade->activeMask & (1 << buffer)) {
            fade->activeMask ^= (u16)(1 << buffer);
        }
    } else if (control->direction == 0) {
        cur = control->cur;
        cur += control->step;
        if (cur > control->end) {
            cur = control->end;
        }
        control->cur = cur;
    } else {
        cur = control->cur;
        cur -= control->step;
        if (cur < control->end) {
            cur = control->end;
        }
        control->cur = cur;
    }
}

void PaletteFade_Transfer(PaletteFade *fade) {
    int i;

    if (fade->transferAll == FALSE && fade->state != PALFADE_STATE_ACTIVE) {
        return;
    }
    for (i = 0; i < PALFADE_BUFFER_COUNT; i++) {
        if (fade->transferAll == FALSE &&
            (fade->buffers[i].faded == NULL || IsBitSet(fade->transferMask, i) == FALSE)) {
            continue;
        }
#ifdef BWBATTLE
        // bwbattle: the BG palettes are Platinum's, and so are the main engine's OBJ palettes but the OBJ extension's
        if (i != PALFADE_BUFFER_MAIN_OBJ) {
            continue;
        }
        GX_LoadOBJPltt(fade->buffers[i].faded, BW_OBJ_EXT_PLTT_OFFSET, fade->buffers[i].size);
        continue;
#endif
        DC_FlushRange(fade->buffers[i].faded, fade->buffers[i].size);
        switch (i) {
        case PALFADE_BUFFER_MAIN_BG:
            GX_LoadBGPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            break;
        case PALFADE_BUFFER_SUB_BG:
            GXS_LoadBGPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            break;
        case PALFADE_BUFFER_MAIN_OBJ:
            GX_LoadOBJPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            break;
        case PALFADE_BUFFER_SUB_OBJ:
            GXS_LoadOBJPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            break;
        case PALFADE_BUFFER_MAIN_BG_EX0:
            GX_BeginLoadBGExtPltt();
            GX_LoadBGExtPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            GX_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_MAIN_BG_EX1:
            GX_BeginLoadBGExtPltt();
            GX_LoadBGExtPltt(fade->buffers[i].faded, 0x2000, fade->buffers[i].size);
            GX_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_MAIN_BG_EX2:
            GX_BeginLoadBGExtPltt();
            GX_LoadBGExtPltt(fade->buffers[i].faded, 0x4000, fade->buffers[i].size);
            GX_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_MAIN_BG_EX3:
            GX_BeginLoadBGExtPltt();
            GX_LoadBGExtPltt(fade->buffers[i].faded, 0x6000, fade->buffers[i].size);
            GX_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_SUB_BG_EX0:
            GXS_BeginLoadBGExtPltt();
            GXS_LoadBGExtPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            GXS_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_SUB_BG_EX1:
            GXS_BeginLoadBGExtPltt();
            GXS_LoadBGExtPltt(fade->buffers[i].faded, 0x2000, fade->buffers[i].size);
            GXS_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_SUB_BG_EX2:
            GXS_BeginLoadBGExtPltt();
            GXS_LoadBGExtPltt(fade->buffers[i].faded, 0x4000, fade->buffers[i].size);
            GXS_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_SUB_BG_EX3:
            GXS_BeginLoadBGExtPltt();
            GXS_LoadBGExtPltt(fade->buffers[i].faded, 0x6000, fade->buffers[i].size);
            GXS_EndLoadBGExtPltt();
            break;
        case PALFADE_BUFFER_MAIN_OBJ_EX:
            GX_BeginLoadOBJExtPltt();
            GX_LoadOBJExtPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            GX_EndLoadOBJExtPltt();
            break;
        case PALFADE_BUFFER_SUB_OBJ_EX:
            GXS_BeginLoadOBJExtPltt();
            GXS_LoadOBJExtPltt(fade->buffers[i].faded, 0, fade->buffers[i].size);
            GXS_EndLoadOBJExtPltt();
            break;
        }
    }
    fade->transferMask = fade->activeMask;
    if (fade->transferMask == 0) {
        fade->state = PALFADE_STATE_IDLE;
    }
}

u16 PaletteFade_GetActiveMask(PaletteFade *fade) {
    return fade->activeMask;
}

void PaletteFade_SetTransferAll(PaletteFade *fade, u32 transferAll) {
    fade->transferAll = transferAll;
}

void PaletteFade_SetAllActive(PaletteFade *fade, u32 active) {
    fade->state = active & 1;
    fade->activeMask = 0x3fff;
}

u16 PaletteFade_GetColor(PaletteFade *fade, u32 buffer, u32 which, u16 pos) {
    if (which == PALFADE_UNFADED) {
        return fade->buffers[buffer].unfaded[pos];
    }
    if (which == PALFADE_FADED) {
        return fade->buffers[buffer].faded[pos];
    }
    return 0;
}

void BlendColors(const u16 *src, u16 *dst, u16 count, u8 fraction, u16 color) {
    // Read through the RGB555 bitfields: shifts and masks don't give the original's code
    const PaletteColor *target = (const PaletteColor *)&color;
    int r = target->r;
    int g = target->g;
    int b = target->b;
    u16 i;

    for (i = 0; i < count; i++) {
        const PaletteColor *rgb = (const PaletteColor *)&src[i];
        int cr = rgb->r;
        int cg = rgb->g;
        int cb = rgb->b;

        dst[i] = GX_RGB(BLEND_CHANNEL(cr, r, fraction), BLEND_CHANNEL(cg, g, fraction), BLEND_CHANNEL(cb, b, fraction));
    }
}

void PaletteFade_BlendBuffer(PaletteFade *fade, u32 buffer, u16 offset, u16 count, u8 fraction, u16 color) {
    BlendColors(fade->buffers[buffer].unfaded + offset, fade->buffers[buffer].faded + offset, count, fraction, color);
}

void PaletteFade_BlendPalettes(PaletteFade *fade, u32 buffer, u16 paletteMask, u8 fraction, u16 color) {
    u32 offset = 0;

    while (paletteMask != 0) {
        if (paletteMask & 1) {
            PaletteFade_BlendBuffer(fade, buffer, offset, 16, fraction, color);
        }
        paletteMask >>= 1;
        offset += 16;
    }
}

void ColorFilter_ApplyLUT(u16 *colors, int count, const u8 *lut) {
    int i;

    for (i = 0; i < count; i++) {
        int red = *colors & 0x1f;
        int green = (*colors >> 5) & 0x1f;
        int blue = (*colors >> 10) & 0x1f;
        u8 gray = lut[(red * 76 + green * 151 + blue * 29) >> 8];

        *colors = (gray << 10) | (gray << 5) | gray;
        colors++;
    }
}

void ColorFilter_ApplyVintage(u16 *colors, int count) {
    int i;

    for (i = 0; i < count; i++) {
        int red = *colors & 0x1f;
        int green = (*colors >> 5) & 0x1f;
        int blue = (*colors >> 10) & 0x1f;
        int gray = (red * 76 + green * 151 + blue * 29) >> 8;
        u32 r = (gray * g_FieldColorPostFXWeightR) >> 8;
        u32 g = (gray * g_FieldColorPostFXWeightG) >> 8;
        u32 b = (gray * g_FieldColorPostFXWeightB) >> 8;

        *colors = (b << 10) | (g << 5) | r;
        colors++;
    }
}

void PaletteFade_ApplyPalette(PaletteFade *fade, u16 palette, u16 buffer) {
    BlendFadeColors(fade->buffers[buffer].unfaded + palette * 16, fade->buffers[buffer].faded + palette * 16,
                    &fade->buffers[buffer].fade, 16);
}
