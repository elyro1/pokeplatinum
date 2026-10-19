#ifndef POKEBW2_PML_MAIL_H
#define POKEBW2_PML_MAIL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// Names from swan
typedef struct MailData {
    u32 trainerId;
    u8 trainerGender;
    u8 region;
    u8 gameVersion;
    u8 unk07;
    u16 trainerName[8];
    u16 unk18[3];
    // 0xffff when there is none
    u16 unk1E;
    PMSData message[3];
} MailData;

#define MAIL_COUNT 20
#define MAIL_DESIGN_COUNT 12
#define MAIL_DESIGN_MAX 11

// Whether the mail is filled in, which is when its design is one of the MAIL_DESIGN_COUNT
BOOL func_020096a8(const MailData *mail);
// Allocates a blank mail
MailData *CreateMailData(HeapID heapId);
// Empties the mail
void ResetMailData(MailData *mail);
void copyMailMsg(const MailData *src, MailData *dest);
// Empties the mail, then sets its design and the author's info from the game data
void func_020096f8(MailData *mail, u32 design, u32 unused, GameData *gameData);
u32 func_02009730(const MailData *mail);
u16 *func_02009734(MailData *mail);
// Sets the mail's author's name
void func_02009738(MailData *mail, const u16 *name);
u8 func_0200974c(const MailData *mail);
u8 func_02009750(const MailData *mail);
void func_02009754(MailData *mail, u32 design);
u8 func_0200975c(const MailData *mail);
u8 func_02009760(const MailData *mail);
u16 func_02009764(const MailData *mail);
void func_02009768(MailData *mail, u16 value);
PMSData *func_0200976c(MailData *mail, u8 index);
void func_0200977c(MailData *mail, const PMSData *src, u8 index);
// The save block of mail, and its size
void *func_02009790(GameData *gameData);
u32 func_020097a0(void);
// Empties all of the mails of a mailbox
void func_020097a8(MailData *mails);
// The mail of the save's mailbox (box 0) or of a Pokémon: the free slot, and clearing, copying and reading a slot's
// mail
s32 func_020097c4(MailData *mails, u32 box);
void func_020097d0(MailData *mails, u32 box, u32 index);
void func_020097e0(MailData *mails, u32 box, u32 index, const MailData *mail);
MailData *func_020097f4(MailData *mails, u32 box, u32 index, HeapID heapId);
s32 func_02009818(MailData *mails, s32 count);
MailData *func_02009844(MailData *mails, u32 box, s32 index);

#endif // POKEBW2_PML_MAIL_H
