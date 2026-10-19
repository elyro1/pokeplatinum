#ifndef POKEBW2_BATTLE_BTL_REC_H
#define POKEBW2_BATTLE_BTL_REC_H

// Overlay 167's btl_rec.c (named by its string): the recording of a battle's actions, and its playback

#include "types.h"
#include "battle/btl_action.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The actions of a turn, as a chunk of the recording: the chunk's header in data[0] and data[1], then the clients in
// clientFlags, each with a byte (client ID in bits 5 to 7, action count in bits 0 to 4) and its actions
typedef struct {
    u8 size;
    u8 clientFlags;
    u8 numClients;
    // The chunk's type and whether it starts a chapter, for its header
    u8 type : 6;
    u8 chapter : 1;
    u8 overflow : 1;
    u8 data[0x3c];
} BtlRecTool;

void func_ov167_021d48a0(BtlRecTool *tool, BOOL chapter);
void *func_ov167_021d48c4(BtlRecTool *tool, u32 *size, BOOL chapter);
void func_ov167_021d48e0(BtlRecTool *tool, u8 clientId, const void *actions, u8 count);
void *func_ov167_021d4958(BtlRecTool *tool, u8 value, u32 *size);
void *func_ov167_021d4990(BtlRecTool *tool, u32 *size);
void func_ov167_021d49a0(BtlRecTool *tool, const void *data, u32 size);
BOOL func_ov167_021d49d0(BtlRecTool *tool, u32 *pos, u8 *clientId, u8 *count, void *actions);

// A recording is a stream of chunks, each starting with a header byte: the count in bits 0 to 3, the chunk type in bits
// 4 to 6 (1 for the clients' actions, 2 for a turn without them, 3 for a timeout) and the start of a chapter in bit 7.
// Each client's actions in a type 1 chunk start with a byte holding the client ID in bits 5 to 7 and the action count
// in bits 0 to 4.

// The reader of a recorded battle's actions, which the main module holds
typedef struct {
    const u8 *data;
    u32 size : 31;
    // Set when the recording ran out or was corrupt; every client gets a blank action from then on
    u32 error : 1;
    // Each client's read position
    u32 pos[4];
    // Each client's actions of the last chunk read
    BattleAction actions[4][16];
} BtlRecReader;

BtlRecorder *func_ov167_021d45b0(HeapID heapId, u32 type);
void func_ov167_021d45e8(BtlRecorder *recorder);
void func_ov167_021d45f0(BtlRecorder *recorder, const void *data, u32 size);
u8 func_ov167_021d4624(const void *data);
void *func_ov167_021d4628(BtlRecorder *recorder, u32 *size);
void func_ov167_021d4630(BtlRecReader *reader, const void *data, u32 size);
void func_ov167_021d4660(BtlRecReader *reader);
BOOL func_ov167_021d4674(BtlRecReader *reader, u8 clientId);
BattleAction *func_ov167_021d46a4(BtlRecReader *reader, u8 clientId, u8 *count, u8 *chapter);
u32 func_ov167_021d481c(BtlRecReader *reader);
BOOL func_ov167_021d4880(BtlRecReader *reader, u8 clientId);

#endif // POKEBW2_BATTLE_BTL_REC_H
