#ifndef POKEBW2_BATTLE_B_PLIST_MAIN_H
#define POKEBW2_BATTLE_B_PLIST_MAIN_H

#include "types.h"
#include "battle/b_app_tool.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/printsys.h"

// The battle party list in overlay 287 (b_plist_main.c and the other b_plist_*.c files, the first named by the ROM's
// embedded string): the party shown during a battle to switch a Pokémon in, see its summary and moves, or pick a
// move to forget, which runs on the evolution demo's tasks with overlay 285 loaded. It sets done when it ends.
// Every name here is ours; swan has none for this overlay.

#define OVERLAY_OV285 OVERLAY_ID(285)
#define OVERLAY_OV287 OVERLAY_ID(287)

typedef struct {
    GameData *gameData;
    PokeParty *party;
    PokeParty *unk8;
    Font *font;
    HeapID heapId;
    u32 unk14;
    BOOL unk18; // TRUE in a multi battle
    u8 unk1C;
    u8 unk1D[2];
    u8 unk1F;
    u8 partyIndex;
    u8 unk21;
    u8 unk22;
    u8 unk23;
    u16 item;
    u16 move;
    TCBManager *tcbManager;
    PaletteFade *paletteFade;
    u32 unk30;
    u32 unk34; // 1 forces the exit
    u8 unk38[6];
    u8 unk3E[2];
    u32 unk40; // 1 plays the sound effects
    u8 *usingKeys;
    u8 unk48[3];
    // The slot of the move to forget, or 4 for none
    u8 slot;
    u8 done;
} BPlistParam;

// A move of a BPlistPokemon
struct BPlistMove {
    u16 move;
    u8 pp;
    u8 maxPp;
    u8 type;
    u8 category;
    u8 accuracy; // 0 when the move always hits
    u8 power;
};

// One Pokémon as the list shows it
struct BPlistPokemon {
    PartyPkm *pkm; // 0x00  NULL for an empty slot
    u16 species;   // 0x04  0 for an empty slot
    u16 attack;    // 0x06
    u16 defense;   // 0x08
    u16 speed;     // 0x0a
    u16 spAttack;  // 0x0c
    u16 spDefense; // 0x0e
    u16 hp;        // 0x10
    u16 maxHp;     // 0x12
    u8 type1;      // 0x14
    u8 type2;      // 0x15
    u8 level : 7;  // 0x16
    u8 hideSex : 1;
    u8 sex : 3; // 0x17
    u8 status : 4;
    u8 isEgg : 1;
    u16 ability;         // 0x18
    u16 item;            // 0x1a
    u32 exp;             // 0x1c
    u32 levelExp;        // 0x20
    u32 nextLevelExp;    // 0x24
    u32 form;            // 0x28
    BPlistMove moves[4]; // 0x2c
}; // 0x4c

// The first of each kind of actor in BPlistWork's actors, which overlays its arrays of them
enum {
    BPLIST_ACTOR_ITEM = 0,
    BPLIST_ACTOR_POKE = 7,
    BPLIST_ACTOR_STATUS = 13,
    BPLIST_ACTOR_UNK1F08 = 19,
    BPLIST_ACTOR_POKE_TYPE = 25,
    BPLIST_ACTOR_MOVE_TYPE = 29,
    BPLIST_ACTOR_CATEGORY = 39,
    BPLIST_ACTOR_MAX = 41,
};

// The list's work, 0x2544 bytes, shared by the overlay's files
struct BPlistWork {
    BPlistParam *param;       // 0x0000
    BPlistPokemon pokemon[6]; // 0x0004
    u8 partyOrder[6];         // 0x01cc  the party slot at each list position
    TCBExManager *tcbExMgr;   // 0x01d4
    PaletteFade *paletteFade; // 0x01d8
    // Screen pieces cut from the list's tilemaps, by button
    u16 plateScrn[2][4][16 * 6];   // 0x01dc  buttons 0-5: [source column][state]
    u16 moveButtonScrn[4][13 * 5]; // 0x07dc  buttons 8-11
    u16 button12Scrn[4][5 * 5];    // 0x09e4
    u16 button13Scrn[4][5 * 5];    // 0x0aac
    u16 button6Scrn[4][5 * 5];     // 0x0b74
    u16 button7Scrn[3][30 * 17];   // 0x0c3c
    u16 button14Scrn[4][16 * 6];   // 0x1830  buttons 14-26
    u16 button27Scrn[3][26 * 5];   // 0x1b30
    u16 button28Scrn[3][5 * 2];    // 0x1e3c  buttons 28-31
    u16 savedPalette[16];          // 0x1e78
    u8 animSeq;                    // 0x1e98  also the swap animation's
    u8 animCount;                  // 0x1e99
    u8 animButton;                 // 0x1e9a
    u8 : 4;                        // 0x1e9b
    u8 animMode : 3;
    u8 animActive : 1;
    Font *smallFont;          // 0x1e9c
    MsgData *msgData;         // 0x1ea0
    WordSet *wordSet;         // 0x1ea4
    StrBuf *strBuf;           // 0x1ea8
    PrintQueue *printQueue;   // 0x1eac
    PrintStream *printStream; // 0x1eb0
    BOOL streamAdvanced;      // 0x1eb4
    ClActUnit *actorUnit;     // 0x1eb8
    union {
        ClActor *actors[BPLIST_ACTOR_MAX]; // 0x1ebc  all of them, created, hidden and freed as one
        struct {
            ClActor *itemIcons[7];      // 0x1ebc  by party slot, then one more for page 2
            ClActor *pokeIcons[6];      // 0x1ed8
            ClActor *statusIcons[6];    // 0x1ef0
            ClActor *unk1f08[6];        // 0x1f08  a getUINarcIdx icon per non-egg member, purpose unknown
            ClActor *pokeTypeIcons[4];  // 0x1f20  two pairs, by pokeTypeIconBuf
            ClActor *moveTypeIcons[10]; // 0x1f30  two sets of five, by typeIconBuf
            ClActor *categoryIcons[2];  // 0x1f58  by categoryIconBuf
        };
    };
    PrintWindow msgWins[2];  // 0x1f60  the info line and the message window
    PrintWindow windows[64]; // 0x1f70  at most 22 used
    BmpWin *dummyWin;        // 0x2170
    u8 windowCount;          // 0x2174
    u8 windowSwap : 5;       // 0x2175
    u8 pokeTypeIconBuf : 1;  // which pair of Pokémon type icons
    u8 typeIconBuf : 1;      // which set of move type icons
    u8 categoryIconBuf : 1;  // which category icon
    u8 startPos;             // 0x2176  the party index at entry
    u8 pageChangeSeq : 4;    // 0x2177
    u8 infoWinFramePending : 4;
    int seq;                 // 0x2178  the state, 0x21 ends
    int nextSeq;             // 0x217c  taken once the button animation ends
    u8 page;                 // 0x2180
    CursorMove *cursorMove;  // 0x2184
    BAppCursor *cursor;      // 0x2188
    BOOL cursorVisible;      // 0x218c
    u8 page1Pos;             // 0x2190
    u8 page6Pos;             // 0x2191
    u8 page7Pos;             // 0x2192
    u8 msgWinOpen;           // 0x2193
    BGWinFrame *plateFrames; // 0x2194
    u8 unk2198[0x300];       // 0x2198  never accessed
    u8 swapPos;              // 0x2498
    u8 swapAnimPos[2];       // 0x2499
    u16 swaps[2][2];         // 0x249c  swapped position pairs, 0xff when empty
    u32 charRes[26];         // 0x24a4
    u32 plttRes[6];          // 0x250c
    u32 cellRes[6];          // 0x2524
    const u8 *flushList;     // 0x253c  window indices ending in 0xff
    BOOL initialized;        // 0x2540
}; // 0x2544

void BPlistMain_Start(BPlistParam *param);

int BPlistMain_CheckPos(BPlistWork *work, int pos);
int BPlistMain_GetSwitchError(BPlistWork *work);
BOOL BPlistMain_IsPartnerSlot(BPlistWork *work, u32 idx);
u8 BPlistMain_GetPartySlot(BPlistWork *work, int pos); // the party slot at a list position
BOOL BPlistMain_PopSwap(BPlistWork *work, u8 *pos1, u8 *pos2, BOOL clear);

#endif // POKEBW2_BATTLE_B_PLIST_MAIN_H
