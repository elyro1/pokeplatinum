#ifndef POKEBW2_GFL_NET_HANDLE_H
#define POKEBW2_GFL_NET_HANDLE_H

#include "types.h"
#include "struct_decls.h"

// A handle for each machine and one for the server: the exchange of MAC addresses and IDs when a machine connects,
// the timing numbers that synchronize the machines, and data a machine shares

#define GFL_NET_HANDLE_MAX 9
#define GFL_NET_NETID_SERVER 0xff

enum {
    NET_HANDLE_STATE_NONE,
    NET_HANDLE_STATE_1,
    NET_HANDLE_STATE_SEND_NEGOTIATION,
    // The machine's negotiation has been accepted
    NET_HANDLE_STATE_NEGOTIATED = 5,
    NET_HANDLE_STATE_REJECTED = 7,
};

// What a machine sends to the server when it connects
typedef struct {
    u8 serverMac[6];
    u8 mac[6];
    u8 netId;
    u8 valid;
    u8 unkE;
    u8 unkF;
} NetNegotiation;

struct NetHandle {
    void *data;
    u32 dataSize;
    NetNegotiation *negotiations[GFL_NET_HANDLE_MAX];
    struct {
        u8 netId;
        u8 pending;
    } replies[8];
    u8 unk3C[2];
    // The machines whose negotiations have been sent
    u16 negotiationsSent;
    u16 serverNegotiationsSent;
    u8 unk42[2];
    u16 timingRecv;
    u16 timingSend;
    u8 unk48;
    u8 timingSendPending;
    u8 state;
    u8 unk4B[3];
    u8 infoPending;
    u8 unk4F;
};

// The machine with a handle, or GFL_NET_NETID_SERVER
int func_020401dc(NetHandle *handle);
// Create and free the handles; the argument is not used
void func_0204034c(void *sys);
void func_020403a4(void *sys);
NetHandle *func_02040414(int netId);
// This machine's handle
NetHandle *func_02040440(void);
BOOL func_0204044c(NetHandle *handle);
u8 func_02040464(NetHandle *handle);
// The machines whose negotiations have been accepted
int func_02040474(void);
void func_0204049c(int netId);
BOOL func_02040504(void);
// Command handlers: a machine's timing number, negotiation, and shared data
void func_02040574(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_020405ec(int netId, int size, const void *data, void *work, NetHandle *handle);
// Sends a timing number to synchronize on, which func_02040654 and func_02040664 check for
void func_020405f8(NetHandle *handle, u16 timing);
void func_02040624(NetHandle *handle, u32 a1, u32 a2);
BOOL func_02040654(NetHandle *handle, u16 timing);
BOOL func_02040664(NetHandle *handle, u32 a1, u32 a2);
void func_020406e0(void);
void func_02040880(int netId, int size, u8 *data, void *work, NetHandle *handle);
void func_0204095c(int netId, int size, NetNegotiation *negotiation, void *work, NetHandle *handle);
void func_02040a20(int netId, int size, u8 *data);
// Forgets a machine, as when it disconnects
void func_02040a9c(int netID);
void func_02040ad0(int netID, int size, const void *data, void *work, NetHandle *handle);
void func_02040b38(NetHandle *handle, int a1, int a2, int a3);
void func_02040b68(NetHandle *handle, int value);

#endif // POKEBW2_GFL_NET_HANDLE_H
