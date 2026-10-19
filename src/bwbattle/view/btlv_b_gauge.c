// A side's gauge of party balls, which slides in at a trainer battle's start. The name is the ROM's string, from
// GFL_HeapAllocate's asserts. The names are ours; swan has none for this file

#include "battle/btlv_b_gauge.h"
#include "types.h"
#include "battle/btl_pokeparam.h"
#include "battle/btlv_effect.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "system/palanm.h"

// A ball, or the gauge's frame as the last, 0x1c bytes
typedef struct {
    ClActor *actor; // 0x00
    s32 slideEndX;  // 0x04  where the slide stops, past restX
    s32 restX;      // 0x08  where it bounces back to
    s32 speed;      // 0x0c  0 once it stopped
    u32 sequence;   // 0x10  its animation once it stopped
    s32 wait;       // 0x14  frames before it starts
    ClActorPos pos; // 0x18
} BtlvBGaugeBall;

struct BtlvBGauge {
    ClActUnit *unit;         // 0x00
    u32 chars;               // 0x04
    u32 cellAnims;           // 0x08
    u32 palette;             // 0x0c
    BtlvBGaugeBall balls[7]; // 0x10  the frame last
    s32 seq;                 // 0xd4  0 slide in, 1 bounce back
    BOOL busy;               // 0xd8
    s32 sePlayer;            // 0xdc  alternates between 0 and 1
    HeapID heapId;           // 0xe0
    BOOL loaded;             // 0xe4
    BtlvBGaugeParam param;   // 0xe8
};

typedef struct {
    BtlvBGauge *gauge;
} BtlvBGaugeTask;

// Where a side's balls start and where its frame rests
typedef struct {
    s16 ballX;
    s16 ballY;
    s16 frameX;
    s16 frameY;
} BtlvBGaugePos;

static void BtlvBGauge_LoadResources(BtlvBGauge *work);
static void BtlvBGauge_FreeResources(BtlvBGauge *work);
static void BtlvBGauge_SlideTask(TCB *tcb, void *data);
static void BtlvBGauge_SlideTaskEnd(TCB *tcb);

static const BtlvBGaugePos data_ov168_021f3e50[2] = { { 166, 112, 184, 120 }, { 90, 40, 56, 48 } };

// The animations by ball state: a ball's while it slides on each side, then once it stopped; column 4 is the frame's
static const u32 data_ov168_021f3e60[3][5] = { { 9, 3, 5, 4, 11 }, { 9, 0, 2, 1, 10 }, { 9, 6, 8, 7, 10 } };

BtlvBGauge *BtlvBGauge_Create(const BtlvBGaugeParam *param, HeapID heapId) {
    BtlvBGauge *work;
    BtlvBGaugeTask *task;

    work = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(BtlvBGauge), TRUE, "btlv_b_gauge.c", 0x7e);
    work->param = *param;
    work->heapId = HEAPID_TAIL(heapId);
    work->unit = func_0204bf1c(7, 0, work->heapId);
    if (param->mode != 2) {
        BtlvBGauge_LoadResources(work);
    }

    task = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(BtlvBGaugeTask), FALSE, "btlv_b_gauge.c", 0x8b);
    task->gauge = work;
    BtlvEffect_AddTask(GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BtlvBGauge_SlideTask, task, 0),
                       BtlvBGauge_SlideTaskEnd, 0);
    work->busy = TRUE;
    GFL_SndSEPlay(SEQ_SE_TB_START);
    return work;
}

void BtlvBGauge_Delete(BtlvBGauge *work) {
    BtlvBGauge_FreeResources(work);
    func_0204bf98(work->unit);
    GFL_HeapFree(work);
}

BOOL BtlvBGauge_IsBusy(BtlvBGauge *work) {
    if (work->busy) {
        return TRUE;
    }
    return FALSE;
}

static void BtlvBGauge_LoadResources(BtlvBGauge *work) {
    ArcTool *arc;
    int i;
    s32 x;
    BtlvBGaugeParam *param;

    arc = GFL_ArcSysCreateFileHandle(11, HEAPID_TAIL(work->heapId));
    work->loaded = TRUE;
    work->chars = func_0204b81c(arc, 0x1cb, FALSE, 0, work->heapId);
    work->cellAnims = func_0204bde0(arc, 0x1cd, 0x1ce, work->heapId);
    work->palette = func_0204bba0(arc, 0x1cc, 0, 0xc0, work->heapId);
    PaletteFade_LoadFromVRAM(BtlvEffect_GetPaletteFade(), 2, func_0204bdc0(work->palette, FALSE) / 2, 0x20);

    {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };

        param = &work->param;
        x = data_ov168_021f3e50[param->side].ballX;
        setup.x = x + (param->side == 0 ? 0x80 : -0x80);
        setup.y = data_ov168_021f3e50[param->side].ballY;
        for (i = 0; i < 6; i++) {
            work->balls[i].actor =
                func_0204c040(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0, work->heapId);
            func_0204c178(work->balls[i].actor, &work->balls[i].pos, 0);
            func_0204c520(work->balls[i].actor, TRUE);
            func_0204c488(work->balls[i].actor, data_ov168_021f3e60[param->side][param->balls[i]]);
            work->balls[i].sequence = data_ov168_021f3e60[2][param->balls[i]];
            work->balls[i].wait = (i + 1) * 6;
            work->balls[i].restX = x;
            work->balls[i].slideEndX = x + (param->side == 0 ? -2 : 2) * (i + 1);
            work->balls[i].speed = param->side == 0 ? -12 : 12;
            x += param->side == 0 ? 15 : -15;
        }

        setup.x = data_ov168_021f3e50[param->side].frameX + (param->side == 0 ? 0x80 : -0x80);
        setup.y = data_ov168_021f3e50[param->side].frameY;
        work->balls[i].actor =
            func_0204c040(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0, work->heapId);
        func_0204c178(work->balls[i].actor, &work->balls[i].pos, 0);
        func_0204c520(work->balls[i].actor, TRUE);
        func_0204c488(work->balls[i].actor, data_ov168_021f3e60[param->side][4]);
        work->balls[i].sequence = data_ov168_021f3e60[param->side][4];
        work->balls[i].wait = 0;
        work->balls[i].restX = work->balls[i].slideEndX = data_ov168_021f3e50[param->side].frameX;
        work->balls[i].speed = param->side == 0 ? -16 : 16;
    }

    GFL_ArcToolFree(arc);
}

static void BtlvBGauge_FreeResources(BtlvBGauge *work) {
    int i;

    if (work->loaded == TRUE) {
        for (i = 0; i < 7; i++) {
            func_0204c108(work->balls[i].actor);
        }
        func_0204b98c(work->chars);
        func_0204bcd0(work->palette);
        func_0204be64(work->cellAnims);
        work->loaded = FALSE;
    }
}

static void BtlvBGauge_SlideTask(TCB *tcb, void *data) {
    BtlvBGaugeTask *task = data;
    BOOL moving = FALSE;
    int i;
    u32 se;

    if (task->gauge->loaded == FALSE) {
        BtlvEffect_EndTask(tcb);
        return;
    }

    switch (task->gauge->seq) {
    case 0:
        for (i = 0; i < 7; i++) {
            if (task->gauge->balls[i].wait) {
                moving = TRUE;
                task->gauge->balls[i].wait--;
            } else if (task->gauge->balls[i].speed) {
                task->gauge->balls[i].pos.x += task->gauge->balls[i].speed;
                moving = TRUE;
                if (task->gauge->balls[i].speed > 0) {
                    if (task->gauge->balls[i].pos.x >= task->gauge->balls[i].slideEndX) {
                        task->gauge->balls[i].pos.x = task->gauge->balls[i].slideEndX;
                        task->gauge->balls[i].speed = 0;
                    }
                } else if (task->gauge->balls[i].pos.x <= task->gauge->balls[i].slideEndX) {
                    task->gauge->balls[i].pos.x = task->gauge->balls[i].slideEndX;
                    task->gauge->balls[i].speed = 0;
                }
                func_0204c140(task->gauge->balls[i].actor, &task->gauge->balls[i].pos, 0);
                if (task->gauge->balls[i].speed == 0) {
                    func_0204c488(task->gauge->balls[i].actor, task->gauge->balls[i].sequence);
                    switch (task->gauge->balls[i].sequence) {
                    case 9:
                        se = SEQ_SE_TB_KARA;
                        break;
                    case 10:
                    case 11:
                        se = 0;
                        break;
                    default:
                        se = SEQ_SE_TB_KON;
                        break;
                    }
                    if (se) {
                        GFL_SEPlayKeepVol(se, task->gauge->sePlayer);
                        task->gauge->sePlayer ^= 1;
                    }
                }
            }
        }
        if (moving == FALSE) {
            task->gauge->seq++;
        }
        break;
    case 1:
        for (i = 0; i < 7; i++) {
            if (task->gauge->balls[i].pos.x < task->gauge->balls[i].restX) {
                task->gauge->balls[i].pos.x += 2;
                if (task->gauge->balls[i].pos.x >= task->gauge->balls[i].restX) {
                    task->gauge->balls[i].pos.x = task->gauge->balls[i].restX;
                }
                moving = TRUE;
            } else if (task->gauge->balls[i].pos.x > task->gauge->balls[i].restX) {
                task->gauge->balls[i].pos.x -= 2;
                if (task->gauge->balls[i].pos.x <= task->gauge->balls[i].restX) {
                    task->gauge->balls[i].pos.x = task->gauge->balls[i].restX;
                }
                moving = TRUE;
            }
            func_0204c140(task->gauge->balls[i].actor, &task->gauge->balls[i].pos, 0);
        }
        if (moving == FALSE) {
            BtlvEffect_EndTask(tcb);
        }
        break;
    }
}

static void BtlvBGauge_SlideTaskEnd(TCB *tcb) {
    BtlvBGaugeTask *task = GFL_TCBGetData(tcb);
    task->gauge->busy = FALSE;
}
