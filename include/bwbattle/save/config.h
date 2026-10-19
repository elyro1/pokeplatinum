#ifndef POKEBW2_SAVE_CONFIG_H
#define POKEBW2_SAVE_CONFIG_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The options, which a save keeps at the start of the block that getTrainerDataBlkAddress returns, so a pointer to one
// is that block's. The field names are guessed, apart from kanji and cgearOn, which the code's use shows
struct Config {
    u16 textSpeed : 4;
    u16 soundFlags : 2;
    u16 unk6 : 1;
    u16 unk7 : 1;
    // The kana or kanji text of the Japanese version, which also sets the message language
    u16 kanji : 1;
    u16 unk9 : 1;
    // Whether the C-Gear is on, which a continue sets from the answer to the start menu's question
    u16 cgearOn : 1;
    u16 : 5;
    u16 unk2;
};

// Points the key remaps at the game's table
void initKeyBlk(void);
Config *func_0200898c(u32 heapId);
// Copies a config into the save's
void initConfig(const Config *config, Config *dest);
void func_020089c0(Config *config);
u32 func_02008a14(const Config *config);
void func_02008a1c(Config *config, u32 value);
u32 getSoundFlags(const Config *config);
void func_02008a38(Config *config, u32 value);
BOOL func_02008a4c(const Config *config);
void func_02008a54(Config *config, u32 value);
BOOL func_02008a68(const Config *config);
void func_02008a70(Config *config, u32 value);
// Also sets the message language
void func_02008a8c(Config *config, u32 value);
u32 func_02008a84(const Config *config);
void func_02008ab4(const Config *config);
u32 func_02008ac8(const Config *config);
void func_02008ad0(Config *config, u32 value);
u32 func_02008ae8(const Config *config);
void func_02008af0(Config *config, u32 value);

#endif // POKEBW2_SAVE_CONFIG_H
