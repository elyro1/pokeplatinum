// The battle view's 2D cell actors: up to three OBJ actors the effects load, move, scale and fade, and in mode 1 a
// turn gauge with a popup that shows a message. The name is the ROM's string, from GFL_HeapAllocate's asserts. The
// names are ours; swan has none for this file

#include "battle/btlv_clact.h"
#include "types.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_string.h"
#include "battle/btlv_effect.h"
#include "gfl/arc.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "system/palanm.h"
#include "system/printsys.h"

// An actor with the resources it was created from
typedef struct {
    ClActor *actor; // 0x0
    u32 chars;      // 0x4
    u32 palette;    // 0x8
    u32 cellAnims;  // 0xc
} BtlvClactEntry;

// 0x1c8 bytes
struct BtlvClact {
    TCBManager *tcbManager;    // 0x000
    ClActUnit *unit;           // 0x004
    BtlvClactEntry entries[3]; // 0x008  the effects' actors
    // 0x038  the turn gauge: its left end, which holds the resources of all the gauge's and the popup's actors, a
    // cell per turn and its right end
    BtlvClactEntry gauge[22];
    BtlvClactEntry popup[2]; // 0x198  the popup and the message drawn into the gauge's characters
    u32 moveFlags;           // 0x1b8  bit n: entries[n] moves, bit 31: the popup shows
    u32 scaleFlags;          // 0x1bc  bit n: entries[n] scales
    u32 mode;                // 0x1c0  func_ov167_0219c988's value. 1: the gauge, 2: no effect actors
    HeapID heapId;           // 0x1c4
};

// A move or scaling of an actor, 0x50 bytes. The popup's task uses it too, with the message in index and its state
// in move.count
typedef struct {
    BtlvClact *wk;        // 0x00
    VecFx32 value;        // 0x04
    int index;            // 0x10
    BtlvEffToolMove move; // 0x14
} BtlvClactMove;

#define POPUP_SHOWING (1 << 31)

static void BtlvClact_InitGauge(BtlvClact *wk);
static void BtlvClact_ExitGauge(BtlvClact *wk);
static void BtlvClact_AddMoveTask(BtlvClact *wk, int index, int type, VecFx32 *start, VecFx32 *end, int frames,
                                  int wait, int count, TCBFunc func, void (*endFunc)(TCB *tcb));
static void BtlvClact_MoveTask(TCB *tcb, void *data);
static void BtlvClact_MoveTaskEnd(TCB *tcb);
static void BtlvClact_ScaleTask(TCB *tcb, void *data);
static void BtlvClact_ScaleTaskEnd(TCB *tcb);
static void BtlvClact_PopupTask(TCB *tcb, void *data);
static void BtlvClact_StartPopupTask(BtlvClact *wk, int number);
static void BtlvClact_PrintPopupMessage(BtlvClact *wk, int number, Font *font);

static const ClActorSetup data_ov168_021f3594 = { 16, 16, 0, 0, 0 };
static const ClActorSetupEx data_ov168_021f359c = { { 0, 0, 0, 0, 1 }, { 0, 0 }, FX32_ONE, FX32_ONE, 0, 1 };
static const ClActorSetupEx data_ov168_021f35b4 = { { 0, 0, 0, 0, 0 }, { 0, 0 }, FX32_ONE, FX32_ONE, 0, 0 };

BtlvClact *BtlvClact_Create(TCBManager *tcbManager, HeapID heapId, u32 mode) {
    BtlvClact *wk = GFL_HeapAllocate(heapId, sizeof(BtlvClact), TRUE, "btlv_clact.c", 0x56);

    wk->tcbManager = tcbManager;
    wk->heapId = heapId;
    wk->mode = mode;
    wk->unit = func_0204bf1c(25, 0, heapId);
    BtlvClact_InitGauge(wk);
    return wk;
}

void BtlvClact_Delete(BtlvClact *wk) {
    int i;

    BtlvClact_ExitGauge(wk);
    for (i = 0; i < 3; i++) {
        if (wk->entries[i].actor != NULL) {
            BtlvClact_DeleteActor(wk, i);
        }
    }
    func_0204bf98(wk->unit);
    GFL_HeapFree(wk);
}

void BtlvClact_Draw(BtlvClact *wk) {
    func_0204b794();
}

int BtlvClact_AddActor(BtlvClact *wk, u32 arcId, u32 fileId, s16 x, s16 y, fx32 scaleX, fx32 scaleY) {
    return BtlvClact_AddActorEx(wk, arcId, fileId, fileId + 1, fileId + 2, fileId + 3, x, y, scaleX, scaleY);
}

int BtlvClact_AddActorEx(BtlvClact *wk, u32 arcId, u32 charFileId, u32 plttFileId, u32 cellFileId, u32 animFileId,
                         s16 x, s16 y, fx32 scaleX, fx32 scaleY) {
    ClActorSetupEx setup = data_ov168_021f359c;
    ArcTool *arc;
    PaletteFade *fade;
    int i;

    if (wk->mode == 2) {
        return 0;
    }

    arc = GFL_ArcSysCreateFileHandle(arcId, HEAPID_TAIL(wk->heapId));
    for (i = 0; i < 3; i++) {
        if (wk->entries[i].actor == NULL) {
            break;
        }
    }
    wk->entries[i].chars = func_0204b81c(arc, charFileId, FALSE, CLACT_VRAM_MAIN, wk->heapId);
    wk->entries[i].palette = func_0204bc48(arc, plttFileId, CLACT_VRAM_MAIN, 0x20 * i + 0x140, wk->heapId);
    wk->entries[i].cellAnims = func_0204bde0(arc, cellFileId, animFileId, wk->heapId);
    fade = BtlvEffect_GetPaletteFade();
    PaletteFade_LoadNCLR(fade, arcId, plttFileId, wk->heapId, PALFADE_BUFFER_MAIN_OBJ, 0x20,
                         (func_0204bdc0(wk->entries[i].palette, FALSE) & 0x3ff) / 2);
    setup.base.x = x;
    setup.base.y = y;
    if (scaleX == 0 && scaleY == 0) {
        wk->entries[i].actor = func_0204c040(wk->unit, wk->entries[i].chars, wk->entries[i].palette,
                                             wk->entries[i].cellAnims, &setup.base, 0, wk->heapId);
    } else {
        setup.scaleX = scaleX;
        setup.scaleY = scaleY;
        wk->entries[i].actor = func_0204c0a4(wk->unit, wk->entries[i].chars, wk->entries[i].palette,
                                             wk->entries[i].cellAnims, &setup, 0, wk->heapId);
    }
    func_0204c520(wk->entries[i].actor, TRUE);
    GFL_ArcToolFree(arc);
    return i;
}

static void BtlvClact_InitGauge(BtlvClact *wk) {
    ClActorSetup setup = data_ov168_021f3594;
    ClActorSetupEx setupEx = data_ov168_021f35b4;
    ArcTool *arc;
    PaletteFade *fade;
    int i;

    if (wk->mode != 1) {
        return;
    }

    arc = GFL_ArcSysCreateFileHandle(11, HEAPID_TAIL(wk->heapId));
    wk->gauge[0].chars = func_0204b81c(arc, 0x22b, FALSE, CLACT_VRAM_MAIN, wk->heapId);
    wk->gauge[0].palette = func_0204bba0(arc, 0x22a, CLACT_VRAM_MAIN, 0x1a0, wk->heapId);
    wk->gauge[0].cellAnims = func_0204bde0(arc, 0x22c, 0x22d, wk->heapId);
    fade = BtlvEffect_GetPaletteFade();
    // BUG: The offset is the first effect actor's palette's, not the gauge's
#ifdef BUGFIX
    PaletteFade_LoadNCLR(fade, 11, 0x22a, wk->heapId, PALFADE_BUFFER_MAIN_OBJ, 0x20,
                         (func_0204bdc0(wk->gauge[0].palette, FALSE) & 0x3ff) / 2);
#else
    PaletteFade_LoadNCLR(fade, 11, 0x22a, wk->heapId, PALFADE_BUFFER_MAIN_OBJ, 0x20,
                         (func_0204bdc0(wk->entries[0].palette, FALSE) & 0x3ff) / 2);
#endif
    for (i = 0; i < 22; i++) {
        setup.x = i * 8;
        wk->gauge[i].actor = func_0204c040(wk->unit, wk->gauge[0].chars, wk->gauge[0].palette, wk->gauge[0].cellAnims,
                                           &setup, 0, wk->heapId);
        func_0204c520(wk->gauge[i].actor, TRUE);
    }
    BtlvClact_SetGauge(wk, 10, 0);

    setupEx.base.x = 128;
    setupEx.base.y = 96;
    setupEx.affineMode = 1;
    wk->popup[0].actor = func_0204c0a4(wk->unit, wk->gauge[0].chars, wk->gauge[0].palette, wk->gauge[0].cellAnims,
                                       &setupEx, 0, wk->heapId);
    func_0204c520(wk->popup[0].actor, TRUE);
    func_0204c124(wk->popup[0].actor, FALSE);
    func_0204c488(wk->popup[0].actor, 20);
    func_0204c438(wk->popup[0].actor, 1);

    setup.x = 128;
    setup.y = 96;
    wk->popup[1].actor = func_0204c040(wk->unit, wk->gauge[0].chars, wk->gauge[0].palette, wk->gauge[0].cellAnims,
                                       &setup, 0, wk->heapId);
    func_0204c520(wk->popup[1].actor, TRUE);
    func_0204c520(wk->popup[1].actor, TRUE);
    func_0204c124(wk->popup[1].actor, FALSE);
    func_0204c488(wk->popup[1].actor, 21);
    GFL_ArcToolFree(arc);
}

void BtlvClact_SetGauge(BtlvClact *wk, int count, int pos) {
    int i;
    int sequence;

    if (pos == 0) {
        func_0204c488(wk->gauge[0].actor, 3);
    } else {
        func_0204c488(wk->gauge[0].actor, 4);
    }
    for (i = 0; i < count; i++) {
        if (i < pos) {
            sequence = 1;
        } else if (i == pos) {
            sequence = 2;
        } else if (i > pos) {
            sequence = 0;
        }
        func_0204c488(wk->gauge[i + 1].actor, sequence);
    }
    func_0204c488(wk->gauge[i + 1].actor, 5);
    for (i += 2; i < 22; i++) {
        func_0204c488(wk->gauge[i].actor, 17);
    }
}

void BtlvClact_DeleteActor(BtlvClact *wk, int index) {
    if (wk->entries[index].actor != NULL) {
        func_0204b98c(wk->entries[index].chars);
        func_0204bcd0(wk->entries[index].palette);
        func_0204be64(wk->entries[index].cellAnims);
        func_0204c108(wk->entries[index].actor);
        wk->entries[index].actor = NULL;
    }
}

static void BtlvClact_ExitGauge(BtlvClact *wk) {
    int i;

    if (wk->mode == 1) {
        for (i = 0; i < 2; i++) {
            func_0204c108(wk->popup[i].actor);
            wk->popup[i].actor = NULL;
        }
        for (i = 0; i < 22; i++) {
            func_0204c108(wk->gauge[i].actor);
            wk->gauge[i].actor = NULL;
        }
        // BUG: These are the first effect actor's resources, which BtlvClact_Delete frees again, and the gauge's
        // are never freed
#ifdef BUGFIX
        func_0204b98c(wk->gauge[0].chars);
        func_0204bcd0(wk->gauge[0].palette);
        func_0204be64(wk->gauge[0].cellAnims);
#else
        func_0204b98c(wk->entries[0].chars);
        func_0204bcd0(wk->entries[0].palette);
        func_0204be64(wk->entries[0].cellAnims);
#endif
    }
}

void BtlvClact_StartMove(BtlvClact *wk, int index, int type, ClActorPos *pos, int frames, int wait, int count) {
    VecFx32 start;
    VecFx32 end;
    ClActorPos current;

    if (wk->entries[index].actor != NULL) {
        end.x = pos->x << FX32_SHIFT;
        end.y = pos->y << FX32_SHIFT;
        end.z = 0;
        func_0204c178(wk->entries[index].actor, &current, CLACT_SURFACE_MAIN);
        start.x = current.x << FX32_SHIFT;
        start.y = current.y << FX32_SHIFT;
        start.z = 0;
        if (type == 1) {
            end.x += start.x;
            end.y += start.y;
            end.z += start.z;
        }
        BtlvClact_AddMoveTask(wk, index, type, &start, &end, frames, wait, count, BtlvClact_MoveTask,
                              BtlvClact_MoveTaskEnd);
        wk->moveFlags |= 1 << index;
    }
}

void BtlvClact_StartScale(BtlvClact *wk, int index, int type, VecFx32 *scale, int frames, int wait, int count) {
    VecFx32 start;

    if (wk->entries[index].actor != NULL) {
        start.x = FX32_ONE;
        start.y = FX32_ONE;
        start.z = FX32_ONE;
        BtlvClact_AddMoveTask(wk, index, type, &start, scale, frames, wait, count, BtlvClact_ScaleTask,
                              BtlvClact_ScaleTaskEnd);
        wk->scaleFlags |= 1 << index;
    }
}

void BtlvClact_SetAnimSeq(BtlvClact *wk, int index, int sequence) {
    if (wk->entries[index].actor != NULL) {
        func_0204c488(wk->entries[index].actor, sequence);
    }
}

void BtlvClact_StartPalFade(BtlvClact *wk, int index, u8 start, u8 end, s8 delay, u16 color) {
    u16 mask;

    if (wk->entries[index].actor != NULL) {
        mask = 1 << ((func_0204bdc0(wk->entries[index].palette, FALSE) & 0x3ff) / 32);
        PaletteFade_StartFade(BtlvEffect_GetPaletteFade(), 1 << PALFADE_BUFFER_MAIN_OBJ, mask, delay, start, end, color,
                              BtlvEffect_GetTCBManager());
    }
}

BOOL BtlvClact_IsPopupShowing(BtlvClact *wk) {
    return wk->moveFlags & POPUP_SHOWING;
}

static void BtlvClact_AddMoveTask(BtlvClact *wk, int index, int type, VecFx32 *start, VecFx32 *end, int frames,
                                  int wait, int count, TCBFunc func, void (*endFunc)(TCB *tcb)) {
    BtlvClactMove *task =
        GFL_HeapAllocate(HEAPID_TAIL(wk->heapId), sizeof(BtlvClactMove), FALSE, "btlv_clact.c", 0x277);

    task->wk = wk;
    task->index = index;
    task->value.x = start->x;
    task->value.y = start->y;
    task->value.z = start->z;
    task->move.type = type;
    task->move.stepTime = frames;
    task->move.stepTimeReset = frames;
    task->move.wait = 0;
    task->move.waitReset = wait;
    task->move.count = count * 2;
    task->move.start.x = start->x;
    task->move.start.y = start->y;
    task->move.start.z = start->z;
    task->move.end.x = end->x;
    task->move.end.y = end->y;
    task->move.end.z = end->z;

    switch (type) {
    case 0:
        break;
    case 1:
        BtlvEffTool_CalcStepVec(&task->move.start, end, &task->move.step, FX32_CONST(frames));
        break;
    case 3:
        task->move.stepTimeReset *= 2;
        task->move.count *= 2;
    case 2:
        task->move.step.x = FX_Div(end->x, FX32_CONST(frames));
        task->move.step.y = FX_Div(end->y, FX32_CONST(frames));
        task->move.step.z = FX_Div(end->z, FX32_CONST(frames));
        break;
    }

    BtlvEffect_AddTask(GFL_TCBMgrAddTask(wk->tcbManager, func, task, 0), endFunc, 0);
}

static void BtlvClact_MoveTask(TCB *tcb, void *data) {
    BtlvClactMove *task = data;
    BtlvClact *wk = task->wk;
    BOOL done = BtlvEffTool_Move(&task->move, &task->value);
    ClActorPos pos;

    pos.x = FX_Whole(task->value.x);
    pos.y = FX_Whole(task->value.y);
    func_0204c140(wk->entries[task->index].actor, &pos, CLACT_SURFACE_MAIN);
    if (done == TRUE) {
        BtlvEffect_EndTask(tcb);
    }
}

static void BtlvClact_MoveTaskEnd(TCB *tcb) {
    BtlvClactMove *task = GFL_TCBGetData(tcb);

    task->wk->moveFlags &= (1 << task->index) ^ 0xffffffff;
}

static void BtlvClact_ScaleTask(TCB *tcb, void *data) {
    BtlvClactMove *task = data;
    BtlvClact *wk = task->wk;
    BOOL done = BtlvEffTool_Move(&task->move, &task->value);
    ClActorScale scale;

    scale.x = task->value.x;
    scale.y = task->value.y;
    func_0204c244(wk->entries[task->index].actor, 1);
    func_0204c270(wk->entries[task->index].actor, &scale);
    if (done == TRUE) {
        BtlvEffect_EndTask(tcb);
    }
}

static void BtlvClact_ScaleTaskEnd(TCB *tcb) {
    BtlvClactMove *task = GFL_TCBGetData(tcb);

    task->wk->scaleFlags &= (1 << task->index) ^ 0xffffffff;
}

static void BtlvClact_PopupTask(TCB *tcb, void *data) {
    BtlvClactMove *task = data;

    switch (task->move.count) {
    case 0:
        if (func_0204c560(task->wk->popup[0].actor) == FALSE) {
            task->move.count++;
        }
        break;
    case 1:
        func_0204c124(task->wk->popup[0].actor, FALSE);
        func_0204c124(task->wk->popup[1].actor, FALSE);
        task->move.count++;
        break;
    case 2:
        // BUG: The task's work is read after it is freed
#ifdef BUGFIX
        task->wk->moveFlags &= ~POPUP_SHOWING;
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
#else
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
        task->wk->moveFlags &= ~POPUP_SHOWING;
#endif
        break;
    case 3:
        break;
    }
}

static void BtlvClact_StartPopupTask(BtlvClact *wk, int number) {
    BtlvClactMove *task =
        GFL_HeapAllocate(HEAPID_TAIL(wk->heapId), sizeof(BtlvClactMove), FALSE, "btlv_clact.c", 0x2f8);

    task->wk = wk;
    task->index = number;
    task->move.wait = 0;
    task->move.count = 0;
    GFL_TCBMgrAddTask(wk->tcbManager, BtlvClact_PopupTask, task, 0);
    wk->moveFlags |= POPUP_SHOWING;
}

void BtlvClact_ShowPopup(BtlvClact *wk, int number, Font *font) {
    func_0204c124(wk->popup[0].actor, TRUE);
    func_0204c56c(wk->popup[0].actor);
    func_0204c124(wk->popup[1].actor, TRUE);
    func_0204c56c(wk->popup[1].actor);
    BtlvClact_PrintPopupMessage(wk, number, font);
    BtlvClact_StartPopupTask(wk, number);
}

// Draws the message into the gauge's characters, centered on a bitmap 8 by 2 tiles
static void BtlvClact_PrintPopupMessage(BtlvClact *wk, int number, Font *font) {
    StrBuf *strbuf = GFL_StrBufCreate(10, wk->heapId);
    GFLBitmap *bitmap = GFL_BitmapCreate(8, 2, 0x20, wk->heapId);
    int width;

    func_ov167_021d4ec0(strbuf, number + 0xc9, 0);
    width = GFL_FontGetBlockWidth(strbuf, font, 0);
    GFL_BitmapFill(bitmap, 0xff);
    GFL_TextRendererDrawToBitmapEx(bitmap, 32 - width / 2, 0, strbuf, font, 0x1d0f);
    func_0204bab8(wk->gauge[0].chars, GFL_BitmapGetPixelData(bitmap), 0x200, 0x680, CLACT_VRAM_MAIN);
    GFL_StrBufFree(strbuf);
    GFL_BitmapFree(bitmap);
}
