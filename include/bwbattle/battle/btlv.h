#ifndef POKEBW2_BATTLE_BTLV_H
#define POKEBW2_BATTLE_BTLV_H

#include "types.h"
#include "battle/b_plist_main.h"
#include "battle/btl_action.h"
#include "battle/btl_pokeparam.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "struct_decls.h"

struct BtlvStringParam {
    u16 message;
    u8 mode;
    u8 type : 4;
    u8 count : 4;
    u32 args[9];
};

// The battle's display: the BG, 2D and 3D systems, and the battle view and AI overlays
extern const BGSysLCDConfig data_ov167_021da8d4;
extern const ClActSysSetup data_ov167_021da8e4;
extern const BGSysVRAMConfig data_ov167_021da900;

void func_ov167_021ce604(HeapID heapId);
void func_ov167_021ce638(void);
void func_ov167_021ce668(HeapID heapId);
void func_ov167_021ce678(HeapID heapId);
void func_ov167_021ce748(void);

void Btlv_StringParam_Setup(BtlvStringParam *param, u32 type, u16 message);
void Btlv_StringParam_AddArg(BtlvStringParam *param, u32 arg);

typedef BOOL (*BtlvMainProc)(BtlvCore *core, s32 *seq, void *work);

// Called while a message prints
typedef BOOL (*BtlvMsgCallback)(u32 arg);

// A procedure of the battle display, which btlv_core.c and btlv_scu.c run the same way: init until it returns TRUE,
// then main
typedef BOOL (*BtlvSubProcFn)(s32 *seq, void *arg);

typedef struct BtlvSubProc {
    BtlvSubProcFn init;
    BtlvSubProcFn main;
    void *arg;
    s32 seq;
} BtlvSubProc;

// The mons a target is chosen among, which the client fills, and the action the display fills in
struct BtlvSelectTargetParam {
    struct {
        BattleMon *mon;
        u8 selectable[4];
    } mons[3];
    BattleAction rotateAction;
    BattleAction action;
};

// btlv_scu.c, the message and command unit of the battle display
BtlvScu *func_ov167_021d0c24(BtlvCore *core, BtlMainModule *mainModule, BtlPokeCon *pokeCon, TCBExManager *tcbManager,
                             Font *font, Font *smallFont, u8 clientId, HeapID heapId);
void func_ov167_021d0cd4(BtlvScu *scu);
void func_ov167_021d0f84(BtlvScu *scu);
void func_ov167_021d0ff8(const BtlvScu *scu);
void func_ov167_021d1084(BtlvScu *scu, BOOL recPlay);
BOOL func_ov167_021d12a0(BtlvScu *scu);
void func_ov167_021d2dbc(BtlvScu *scu, const StrBuf *strbuf);
void func_ov167_021d2e20(BtlvScu *scu, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback);
BOOL func_ov167_021d2ec4(BtlvScu *scu);
BOOL func_ov167_021d2edc(BtlvScu *scu);
void func_ov167_021d3094(BtlvScu *scu, u8 attackerPos, u8 targetPos, u16 move, s32 arg4, u32 arg5, u8 arg6);
BOOL func_ov167_021d3130(BtlvScu *scu);
void func_ov167_021d3188(BtlvScu *scu, u8 pos, u16 move, BOOL arg3);
BOOL func_ov167_021d31d0(BtlvScu *scu);
void func_ov167_021d31e8(BtlvScu *scu, u8 pos, u16 move);
BOOL func_ov167_021d3200(BtlvScu *scu);
void func_ov167_021d3214(BtlvScu *scu, u8 pos, BOOL arg2);
BOOL func_ov167_021d323c(BtlvScu *scu);
void func_ov167_021d3250(BtlvScu *scu, u8 pos);
BOOL func_ov167_021d3284(BtlvScu *scu);
void func_ov167_021d3298(BtlvScu *scu, u8 pos, u16 effect, BOOL immediate);
BOOL func_ov167_021d32f4(BtlvScu *scu);
void func_ov167_021d3354(BtlvScu *scu, u8 pos, u8 clientId, u8 monId, BOOL noEffect);
BOOL func_ov167_021d33d0(BtlvScu *scu);
void func_ov167_021d3414(BtlvScu *scu, u8 pos, BOOL arg2);
BOOL func_ov167_021d3450(BtlvScu *scu);
void func_ov167_021d3464(BtlvScu *scu, u8 pos1, u8 pos2);
BOOL func_ov167_021d3490(BtlvScu *scu, u8 pos1, u8 pos2);
void func_ov167_021d34bc(BtlvScu *scu, u8 pos);
BOOL func_ov167_021d34d4(BtlvScu *scu, u8 pos);
void func_ov167_021d34ec(BtlvScu *scu, u8 viewPos);
BOOL func_ov167_021d34fc(BtlvScu *scu, u8 viewPos);
void func_ov167_021d3510(BtlvScu *scu, u8 pos, u32 arg2);
BOOL func_ov167_021d3558(BtlvScu *scu);
void func_ov167_021d35e0(BtlvScu *scu, u32 arg1, u32 index, BOOL flag);
void func_ov167_021d363c(BtlvScu *scu, u32 index, BOOL flag);
BOOL func_ov167_021d3694(BtlvScu *scu);
void func_ov167_021d3830(BtlvScu *scu);
BOOL func_ov167_021d385c(BtlvScu *scu);
void func_ov167_021d39e4(BtlvScu *scu, u8 pos, BOOL flash);
BOOL func_ov167_021d3a1c(BtlvScu *scu, u8 pos);
void func_ov167_021d3a38(BtlvScu *scu, u8 pos);
BOOL func_ov167_021d3a68(BtlvScu *scu, u8 pos);
void func_ov167_021d3a88(BtlvScu *scu, u8 pos);
BOOL func_ov167_021d3aa8(BtlvScu *scu, u8 pos);
void func_ov167_021d4194(BtlvScu *scu);
BOOL func_ov167_021d41b0(BtlvScu *scu);
void func_ov167_021d41b8(BtlvScu *scu);
void func_ov167_021d41bc(BtlvScu *scu);
void func_ov167_021d41d0(BtlvScu *scu);
BOOL func_ov167_021d41f0(BtlvScu *scu);
void func_ov167_021d41f8(BtlvScu *scu, BattleMon *mon, const BattleMonLevelUp *levelUp);
BOOL func_ov167_021d4234(BtlvScu *scu);
void func_ov167_021d428c(BtlvScu *scu);
BOOL func_ov167_021d42ac(BtlvScu *scu);
void func_ov167_021d4304(BtlvScu *scu);
BOOL func_ov167_021d4324(BtlvScu *scu);

// The battle's selection screens, which share an address range with the bag
void BScenarioMain_Start(void *param);
void BChoiceMain_Start(void *param);

BtlvCore *BtlvCore_Create(BtlMainModule *mainModule, BtlClient *client, BtlPokeCon *pokeCon, u32 arg3, HeapID heapId);
void func_ov167_021ce668(HeapID heapId);
void func_ov167_021ce870(BtlvCore *viewCore);

void func_ov167_021ce8c8(BtlvCore *viewCore);

void func_ov167_021ce8dc(BtlvCore *core, u32 cmd);
BOOL func_ov167_021ce90c(BtlvCore *core);
void *func_ov167_021ce93c(BtlvCore *core, u32 size);
BOOL func_ov167_021ce940(BtlvCore *core, s32 *seq, void *work);
BOOL func_ov167_021cea24(BtlvCore *core, s32 *seq, void *work);
BattleMon *func_ov167_021ceec0(BtlvCore *core, u32 index);
BOOL func_ov167_021ceed8(BtlvCore *core, s32 *seq, void *work);
BOOL func_ov167_021cef50(BtlvCore *core, s32 *seq, void *work);
void func_ov167_021cefb4(BtlvCore *core, BtlvMainProc proc);
void func_ov167_021cefbc(BtlvCore *core);
BOOL func_ov167_021cefc4(BtlvCore *core);
void func_ov167_021cefec(BtlvCore *core, BattleMon *mon, u32 arg2, void *arg3);
void func_ov167_021cf028(BtlvCore *core);
u32 func_ov167_021cf030(BtlvCore *core);
void func_ov167_021cf048(BtlvCore *core, BattleMon *mon, void *arg2);
void func_ov167_021cf094(BtlvCore *core, BtlvSelectTargetParam *param, void *arg2);
void func_ov167_021cf0e4(BtlvCore *core);
BOOL func_ov167_021cf110(BtlvCore *core, s32 *seq, void *work);
void func_ov167_021cf138(BtlvCore *core);
BOOL func_ov167_021cf140(BtlvCore *core);
void func_ov167_021cf148(BtlvCore *core, BattleMon *mon, void *arg2);
u32 func_ov167_021cf174(BtlvCore *core);
void func_ov167_021cf1ac(BtlvCore *core);
void func_ov167_021cf1b4(BtlvCore *core);
void func_ov167_021cf1e0(BtlvCore *core);
void func_ov167_021cf1f0(BtlvCore *core);
BOOL func_ov167_021cf200(BtlvCore *core);
BOOL func_ov167_021cf210(BtlvCore *core);
void func_ov167_021cf234(BtlvCore *core, BPlistParam *param, u8 mode, u8 partyIndex, u32 move);
void BattleClientCmd_StartPokeList(BtlvCore *core, const BtlvPokeListCmd *cmd, s32 partyIndex, u16 move,
                                   BtlvPokeSelectParam *select);
void BattleClientCmd_QuitPokeSelect(BtlvCore *core);
BOOL BattleClientCmd_WaitPokeSelect(BtlvCore *core);
void BattleClientCmd_StartMoveInfoView(BtlvCore *core, u8 partyIndex, u8 slot);
void BattleClientCmd_StartItemSelect(BtlvCore *core, u32 mode, u8 arg2, u8 arg3, u8 canUseItems, u8 arg5);
void func_ov167_021cf72c(BtlvCore *core);
BOOL func_ov167_021cf73c(BtlvCore *core);
u16 func_ov167_021cf8d8(BtlvCore *core);
u8 func_ov167_021cf8ec(BtlvCore *core);
u8 func_ov167_021cf900(BtlvCore *core);
u8 func_ov167_021cf908(BtlvCore *core);
void func_ov167_021cf914(BtlvCore *core);
void func_ov167_021cf95c(BtlvCore *core, u32 arg1);
void func_ov167_021cf9c0(BtlvCore *core);
BOOL func_ov167_021cf9cc(BtlvCore *core);
void func_ov167_021cfaec(BtlvCore *core);
void func_ov167_021cfb28(BtlvCore *core, u32 arg1, u32 *arg2);
void func_ov167_021cfb34(BtlvCore *core, u32 index, StrBuf *value);
BOOL func_ov167_021cfb40(BtlvCore *core);
void func_ov167_021cfc60(BtlvCore *core, u8 attackerPos, u8 targetPos, u16 move, s32 arg4, u32 arg5, u8 arg6);
BOOL func_ov167_021cfca4(BtlvCore *core);
void func_ov167_021cfcb4(BtlvCore *core, u16 arg1, u8 pos, u8 type);
BOOL func_ov167_021cfce0(BtlvCore *core);
BOOL func_ov167_021cfd20(s32 *seq, void *arg);
void func_ov167_021cfd78(BtlvCore *core, u16 arg1, u8 pos, u32 type);
BOOL func_ov167_021cfdb0(BtlvCore *core);
BOOL func_ov167_021cfdf0(s32 *seq, void *arg);
void func_ov167_021cfe40(BtlvCore *core, u16 count, u32 type, const u8 *monIds, u16 arg4);
BOOL func_ov167_021cfe7c(BtlvCore *core);
void func_ov167_021cfeec(BtlvCore *core, u32 type);
void func_ov167_021cff14(BtlvCore *core, u8 pos, u32 visible);
void func_ov167_021cff44(BtlvCore *core, u8 pos);
BOOL func_ov167_021cff60(BtlvCore *core);
void func_ov167_021cff78(BtlvCore *core, u32 arg1, u16 arg2);
BOOL func_ov167_021cff94(BtlvCore *core);
void func_ov167_021cffa8(BtlvCore *core, u32 arg1, u32 arg2, u16 arg3);
BOOL func_ov167_021cffb8(BtlvCore *core);
void func_ov167_021cffcc(BtlvCore *core, u8 pos);
BOOL func_ov167_021cffe8(BtlvCore *core);
void func_ov167_021cfff8(BtlvCore *core, u8 pos);
BOOL func_ov167_021d0008(BtlvCore *core);
void func_ov167_021d0018(BtlvCore *core, u8 pos, u16 arg2);
BOOL func_ov167_021d0038(BtlvCore *core);
void func_ov167_021d0048(BtlvCore *core, u8 pos, u8 arg2, u8 arg3);
BOOL func_ov167_021d0084(BtlvCore *core);
BOOL func_ov167_021d00c4(s32 *seq, void *arg);
void func_ov167_021d011c(const BtlvStringParam *param, StrBuf *strbuf);
void func_ov167_021d01bc(BtlvCore *core, const StrBuf *src);
void func_ov167_021d01c8(BtlvCore *core, const BtlvStringParam *param);
void func_ov167_021d01d8(BtlvCore *core);
void func_ov167_021d01ec(BtlvCore *core, const BtlvStringParam *param);
void func_ov167_021d0210(BtlvCore *core, const BtlvStringParam *param, BtlvMsgCallback callback);
void func_ov167_021d0234(BtlvCore *core, const BtlvStringParam *param);
void func_ov167_021d0250(BtlvCore *core, u16 message, const u32 *args);
void func_ov167_021d026c(BtlvCore *core, u16 message, const u32 *args);
BOOL func_ov167_021d0288(BtlvCore *core, u32 trainerId, u32 msgId);
void func_ov167_021d02cc(BtlvCore *core, u8 monId, u16 move);
BOOL func_ov167_021d02e8(BtlvCore *core);
BOOL func_ov167_021d02f8(BtlvCore *core);
void func_ov167_021d0308(BtlvCore *core, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback);
void func_ov167_021d0330(BtlvCore *core, const StrBuf *strbuf);
void func_ov167_021d0340(BtlvCore *core, u8 pos, BOOL flash);
BOOL func_ov167_021d0350(BtlvCore *core, u8 pos);
void func_ov167_021d0360(BtlvCore *core, u8 pos);
BOOL func_ov167_021d0370(BtlvCore *core, u8 pos);
void func_ov167_021d0380(BtlvCore *core, u8 pos);
BOOL func_ov167_021d0390(BtlvCore *core, u8 pos);
void func_ov167_021d03a0(BtlvCore *core, u32 arg1);
void func_ov167_021d03b0(BtlvCore *core, u32 arg1);
BOOL func_ov167_021d03c0(BtlvCore *core, u32 arg1);
void func_ov167_021d03d4(BtlvCore *core);
BOOL func_ov167_021d03ec(BtlvCore *core);
void func_ov167_021d03fc(BtlvCore *core);
void func_ov167_021d0410(BtlvCore *core, u8 pos);
BOOL func_ov167_021d0428(BtlvCore *core, u8 pos);
void func_ov167_021d0440(BtlvCore *core, u8 pos, u32 arg2);
BOOL func_ov167_021d0450(BtlvCore *core);
void func_ov167_021d0460(BtlvCore *core, u32 arg1);
BOOL func_ov167_021d047c(BtlvCore *core);
void func_ov167_021d048c(BtlvCore *core, u32 arg1, u32 arg2);
BOOL func_ov167_021d04ac(BtlvCore *core);
void func_ov167_021d04bc(BtlvCore *core);
BOOL func_ov167_021d04cc(BtlvCore *core);
void func_ov167_021d04dc(BtlvCore *core, u8 clientId, u8 arg2, u8 arg3, u8 slot1, u8 slot2);
BOOL func_ov167_021d0528(BtlvCore *core);
BOOL func_ov167_021d0568(s32 *seq, void *arg);
void func_ov167_021d05d4(BtlvCore *core, u8 arg1);
BOOL func_ov167_021d0608(BtlvCore *core, u8 *out);
void func_ov167_021d0630(BtlvCore *core);
void func_ov167_021d0640(BtlvCore *core, u8 clientId, u8 arg2);
BOOL func_ov167_021d06ac(BtlvCore *core);
BOOL func_ov167_021d06ec(s32 *seq, void *arg);
void func_ov167_021d0798(BtlvCore *core, const BtlvStringParam *param1, const BtlvStringParam *param2, u32 arg3);
BOOL func_ov167_021d0828(BtlvCore *core, u32 *answer);
void func_ov167_021d0838(BtlvCore *core, u8 partyIndex, u16 move);
BOOL func_ov167_021d0854(BtlvCore *core, u8 *slot);
void func_ov167_021d0978(BtlvCore *core, BattleMon *mon, const BattleMonLevelUp *levelUp);
BOOL func_ov167_021d0988(BtlvCore *core);
void func_ov167_021d0998(BtlvCore *core);
BOOL func_ov167_021d09a8(BtlvCore *core);
void func_ov167_021d09b8(BtlvCore *core);
BOOL func_ov167_021d09c8(BtlvCore *core);
void func_ov167_021d09d8(BtlvCore *core);
BOOL func_ov167_021d09e8(BtlvCore *core);
void func_ov167_021d09f8(BtlvCore *core);
BOOL func_ov167_021d0a08(BtlvCore *core);
void BattleClientCmd_ForceQuitInputNotify(BtlvCore *core);
BOOL BattleClientCmd_ForceQuitInputWait(BtlvCore *core);
u32 func_ov167_021d0a38(BtlvCore *core);
void func_ov167_021d0a48(BtlvCore *core, u16 arg1, u16 arg2);
void func_ov167_021d0a7c(BtlvCore *core, u16 arg1);
void func_ov167_021d0aa0(BtlvCore *core, u32 arg1, u32 arg2);
void func_ov167_021d0ad4(BtlvCore *core, u32 se);
void func_ov167_021d0b4c(BtlvStringParam *param, u8 mode);
void func_ov167_021d0b50(BtlvCore *core, u32 arg1, u32 arg2);
void func_ov167_021d0b70(BtlvCore *core, u32 arg1);
BOOL func_ov167_021d0b90(BtlvCore *core);
void func_ov167_021d0bac(BtlvCore *core, u32 arg1, u32 arg2, u32 arg3);
u32 func_ov167_021d0be4(BtlvCore *core);
void func_ov167_021d0bf4(BtlvCore *core, u32 arg1);

#endif // POKEBW2_BATTLE_BTLV_H
