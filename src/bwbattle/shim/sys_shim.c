#include "bwbattle.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "types.h"
#include "assets/bw2_assets.h"
#include "constants/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "shim/bw_shim.h"
#include "system/app_menu_common.h"

// What BW2's main loop provides the view (system/gfl_use.c): the V-blank and H-blank task managers, the random number
// generator and the assertion handler. The Platinum side calls BwBattle_VBlank and BwBattle_HBlank from its own
// interrupts while bwbattle runs.

#define BW_VBLANK_TASKS 64
#define BW_HBLANK_TASKS 16

static TCBManager *sVBlankTCBMgr;
static TCBManager *sHBlankTCBMgr;
static BOOL sInitialized;

bool BwBattle_Init(void) {
    if (sInitialized) {
        return true;
    }
    if (!BwBattle_AssetsReady()) {
        return false;
    }
    BwAssets_InitArcSys();
    initTableArea(HEAPID_SYSTEM);
    GFL_RandomUpdateMT((u32)time(NULL));
    sVBlankTCBMgr = GFL_TCBMgrCreate(BW_VBLANK_TASKS,
                                     GFL_HeapAllocate(HEAPID_SYSTEM, GFL_TCBMgrCalcAllocSize(BW_VBLANK_TASKS), TRUE,
                                                      __FILE__, __LINE__));
    sHBlankTCBMgr = GFL_TCBMgrCreate(BW_HBLANK_TASKS,
                                     GFL_HeapAllocate(HEAPID_SYSTEM, GFL_TCBMgrCalcAllocSize(BW_HBLANK_TASKS), TRUE,
                                                      __FILE__, __LINE__));
    sInitialized = TRUE;
    return true;
}

void BwBattle_VBlank(void) {
    if (!sInitialized) {
        return;
    }
    GFL_TCBMgrUpdate(sVBlankTCBMgr);
    GFL_BGSysUpdate();
}

void BwBattle_HBlank(void) {
    if (sInitialized) {
        GFL_TCBMgrUpdate(sHBlankTCBMgr);
    }
}

TCB *GFL_VBlankTCBAdd(TCBFunc func, void *data, u32 priority) {
    return GFL_TCBMgrAddTask(sVBlankTCBMgr, func, data, priority);
}

TCB *GFL_HBlankTCBAdd(TCBFunc func, void *data, u32 priority) {
    return GFL_TCBMgrAddTask(sHBlankTCBMgr, func, data, priority);
}

TCBManager *GFL_VBlankGetTCBMgr(void) {
    return sVBlankTCBMgr;
}

void BwShim_Log(const char *format, ...) {
    va_list args;

    fputs("[bwbattle] ", stderr);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}

void GFL_DebugAssertFail(const char *file, u32 line, const char *expression) {
    (void)file;
    (void)line;
    BwShim_Log("assertion failed: %s", expression);
}

void GFL_DebugAssertFailEx(const char *file, u32 line, const char *format, ...) {
    va_list args;

    (void)file;
    (void)line;
    fputs("[bwbattle] assertion failed: ", stderr);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}

// The view reads the held buttons only for debug shortcuts, and the touch panel only for BW2's touch menu
u32 GCTX_HIDGetPressedKeys(void) {
    return 0;
}

BOOL func_0203da48(void) {
    return FALSE;
}

// Communication: the view asks whether a link battle's network is up, sets the BGM tracks' volume for it, and draws
// the wireless icon. bwbattle runs Platinum's local battles
BOOL func_02011844(void) {
    return FALSE;
}

void func_02011bdc(u16 trackMask, s32 volume) {
    (void)trackMask;
    (void)volume;
}

void func_02042ee0(int x, int y) {
    (void)x;
    (void)y;
}

// The file indices of system/app_menu_common.c, in ARCID_APP_MENU_COMMON: the status icons' characters, palette,
// cells and animations, and the type icons'
u32 getUINarcIdx(void) {
    return ARCID_APP_MENU_COMMON;
}

u32 func_0202d8b0(void) {
    return 0xb;
}

u32 func_0202d8b4(void) {
    return 0xc;
}

u32 func_0202d8b8(u32 mapping) {
    return 0xd + mapping;
}

u32 func_0202d8bc(u32 mapping) {
    return 0x10 + mapping;
}

u32 func_0202d90c(u32 mapping) {
    return 0x51 + mapping;
}

u32 func_0202d910(u32 mapping) {
    return 0x54 + mapping;
}

u32 func_0202d914(u32 mapping) {
    return 0xa6 + mapping;
}

u32 func_0202d918(u32 mapping) {
    return 0xa9 + mapping;
}

