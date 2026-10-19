#ifndef POKEBW2_BATTLE_BTL_STRING_H
#define POKEBW2_BATTLE_BTL_STRING_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"

// Overlay 167's btl_string.c, a guessed name: the battle's message strings, which it loads and fills with the
// names and numbers their arguments give

void func_ov167_021d4c64(BtlMainModule *mainModule, u8 clientId, const BtlPokeCon *pokeCon, HeapID heapId);
void func_ov167_021d4d50(void);
// A standard message with count arguments that follow
void func_ov167_021d4ec0(StrBuf *strbuf, u16 message, u32 count, ...);
void func_ov167_021d4f1c(StrBuf *strbuf, u16 message, const u32 *args);
void func_ov167_021d4f90(StrBuf *strbuf, u16 message, const u32 *args);
// "<Pokémon> used <move>!"
void func_ov167_021d5684(StrBuf *strbuf, u8 monId, u16 move);
void func_ov167_021d5700(StrBuf *strbuf, u16 arg1, u32 arg2);
void func_ov167_021d575c(StrBuf *strbuf, u16 message);
void func_ov167_021d5770(StrBuf *strbuf, u16 message, const u32 *args);
void func_ov167_021d57b0(StrBuf *strbuf, u16 message, const u32 *args);
// The six values of a level up's stat window, two digits each
void func_ov167_021d57e4(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense, int speed);
// The same with three digits each
void func_ov167_021d5874(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense, int speed);
void func_ov167_021d5904(StrBuf *strbuf, u32 message);
void func_ov167_021d5924(StrBuf *strbuf, u16 message);
void func_ov167_021d5944(void);

#endif // POKEBW2_BATTLE_BTL_STRING_H
