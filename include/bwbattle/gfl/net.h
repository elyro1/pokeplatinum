#ifndef POKEBW2_GFL_NET_H
#define POKEBW2_GFL_NET_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/net_handle.h"
#include "struct_decls.h"

// The unnamed functions below are from the network library. Some appear to synchronize with the other player or
// toggle error checks, but that is not confirmed.

// How the game set up the network: what net.c keeps from GFL_NetInit
typedef struct {
    // The game's commands, a NetCommand table
    const void *commandTable;
    int commandNum;
    // Called when a machine disconnects
    void (*disconnectCallback)(void *work, int netId);
    // Called when a machine's negotiation is accepted
    void (*negotiationCallback)(void *work, int netId);
    // The data this machine shares with the others, and its size
    void *(*getInfo)(void *work);
    int (*getInfoSize)(void *work);
    void *(*unk18)(void *work);
    int (*unk1C)(void *work);
    // Whether a beacon's game is one to join, a BOOL (*)(u8 gameCommandBase, u8 beaconCommandBase, void *work) that the
    // games give with fewer parameters
    void *unk20;
    void (*unk24)(NetHandle *handle, int a1, void *work);
    void (*unk28)(NetHandle *handle, int a1, void *work);
    void (*unk2C)(void *work);
    void (*unk30)(void *work);
    // 12 bytes that the games' init data zero, the last four of which are a function that dwc_rap.c calls with each
    // Wi-Fi library error it queues (its code and its second value), and the game's work
    union {
        u8 unk34[0xc];
        struct {
            u8 unk34_0[8];
            void (*callback)(int code, int param, void *work);
        } wifiError;
    } unk34;
    int (*unk40)(void *work);
    int (*unk44)(void *work);
    // The size of the heap for Wi-Fi
    u32 wifiHeapSize;
    // The game's ID in the wireless beacons (gameId, of which the low 16 bits go in them) after four more bytes. The
    // games' init data give the bytes
    union {
        u8 unk4C[8];
        struct {
            u32 unk4C_0;
            u32 gameId;
        } id;
    } unk4C;
    HeapID parentHeapId;
    HeapID heapId;
    HeapID wifiHeapId;
    HeapID ircHeapId;
    // Where the wireless signal icon is
    u16 iconX;
    u16 iconY;
    u8 maxConnectNum;
    // The largest packet
    u8 maxSendSize;
    u8 unk62;
    u8 unk63;
    // Whether the parent relays every machine's data (MP mode)
    u8 bMPMode;
    // A GFL_NET_TYPE_*
    u8 bNetType;
    u8 unk66;
    // The high byte of the game's commands
    u8 gameCommandBase;
    // Four bytes that the games give, which overlay 175 sets as one word
    union {
        u8 bytes[4];
        u32 word;
    } unk68;
    u16 unk6C;
    u16 unk6E;
} GFLNetInitData;

// The most machines that can connect
#define GFL_NET_MACHINE_MAX 8

// The kinds of connection: Wi-Fi through DWC, and the others through the wireless or infrared devices
#define GFL_NET_TYPE_WIFI 1
#define GFL_NET_TYPE_WIFI_LOBBY 2
#define GFL_NET_TYPE_WIFI_GTS 6

// What the network device does, wireless or Wi-Fi; the code calls each through GFLNetSys
typedef BOOL (*GFLNetRecvFunc)(u16 netId, u8 *data, u16 size);
typedef BOOL (*GFLNetSendDoneFunc)(BOOL ok);

typedef struct {
    void (*unk00)(int a0, int a1);
    // Starts the device; the callback, which the wireless device calls when its helper ends, is NULL from the net
    BOOL (*init)(HeapID heapId, void *sys, BOOL (*callback)(BOOL success), void *work);
    BOOL (*unk08)(void (*callback)(BOOL ok));
    // Runs the device each frame with the connected machines, returning a negative error or a status
    int (*update)(u16 connectBits);
    BOOL (*unk10)(int a0, int a1);
    BOOL (*unk14)(void (*callback)(BOOL ok));
    BOOL (*unk18)(void);
    BOOL (*unk1C)(void);
    BOOL (*unk20)(void);
    BOOL (*unk24)(void);
    // A found beacon's data and MAC address, by index
    void *(*unk28)(int index);
    u8 *(*unk2C)(int index);
    BOOL (*unk30)(void);
    BOOL (*unk34)(void);
    BOOL (*unk38)(int a0);
    BOOL (*setDisconnectCallback)(void (*callback)(int netId));
    int (*unk40)(int a0, BOOL (*callback)(void));
    BOOL (*unk44)(int a0, int a1);
    int (*unk48)(BOOL a0, const u8 *mac, int a2, int a3, void (*callback)(void));
    BOOL (*unk4C)(int a0, int a1, int a2, int a3);
    BOOL (*unk50)(int a0, int a1, int a2);
    BOOL (*unk54)(BOOL a0, int a1);
    BOOL (*unk58)(void);
    BOOL (*unk5C)(u8 *data);
    u8 *(*getRecvData)(int netId);
    BOOL (*send)(u8 *data, int size, int a2, GFLNetSendDoneFunc callback);
    BOOL (*setRecvCallback)(GFLNetRecvFunc callback);
    BOOL (*unk6C)(void);
    BOOL (*isConnected)(void);
    BOOL (*unk74)(void);
    BOOL (*unk78)(void);
    int (*getConnectBits)(void);
    int (*getNetId)(void);
    int (*getSignalLevel)(void);
    int (*isError)(void);
    void (*unk8C)(int a0);
    int (*unk90)(int a0);
    int (*unk94)(void);
    int (*unk98)(void);
    void (*unk9C)(void);
    BOOL (*unkA0)(void);
    BOOL (*unkA4)(void);
    BOOL (*unkA8)(void);
    BOOL (*unkAC)(void);
    BOOL (*unkB0)(void);
    void (*unkB4)(void);
    void (*unkB8)(int a0);
    BOOL (*unkBC)(void);
    void (*unkC0)(int a0);
    void (*unkC4)(int a0);
    void (*unkC8)(void *a0);
} GFLNetDevTable;

// The network library's state, with a copy of the game's init data
typedef struct {
    GFLNetInitData aNetInit;
    NetHandle handles[GFL_NET_HANDLE_MAX];
    const GFLNetDevTable *pDevTable;
    u8 unk344[4];
    // Called when the network has ended
    void (*exitCallback)(void *work);
    void *devWork;
    u8 unk350[2];
    u8 unk352;
    u8 unk353;
} GFLNetSys;

// The network error, which func_020424ac records
typedef struct {
    int code;
    u32 unk4;
    u32 unk8;
    // Nonzero once an error is recorded, the line it was found on for net_system.c's
    int type;
} GFLNetErrorInfo;

void func_020410dc(void);
// Command handlers of the other parts of the library
void func_02040d78(int netId, int sender, int command, int size, void *data, NetHandle *handle);
BOOL func_02040dc0(int command);
BOOL func_02040dd4(int command);
void *func_02040de8(int command, int netId, int size);
BOOL func_02040c94(int netId);

// net_err.c: the network error screen. The error's code is its message in TEXT_BANK_WIFI_ERROR
// Creates the network's heap, and the buffers the error screen saves the display in
void GFL_NetHeapInit(void);
void GFL_NetErrInit(u32 heapId);
// Finds the network's error, each frame
void func_02011c78(void);
// What the game is to do about the network's error, if there is one: 1 to go on, 2 to end the connection
BOOL GFL_NetErrCheck(void);
void GFL_NetErrMarkShown(void);
BOOL GFL_NetErrShow(u32 a0);
// Shows the fatal error screen for a Wi-Fi error with the message given
void func_02011d04(u32 message);
// GFL_NetErrShow(1)
void func_02011d20(void);
// Records a Wi-Fi error with the message given, to show it
void func_020120f0(u32 message);
// The message for a server's result
u32 func_02011d2c(int result);
// Whether func_02011de0 leaves the screens dark, for the caller to set the brightness with func_02011da4
void func_02011d8c(BOOL keepDark);
void func_02011d98(void);
void func_02011da4(void);
// Shows the error screen until A or B is pressed, with the wireless icon
void func_02011de0(void);
// Whether the error was handled, after shutting the connection down
BOOL func_02012154(void);
void func_02012144(void);
// The state of the wireless connections, as bits: 0x2 a local wireless one, 0x3c the signal, 0x3c0 Wi-Fi
u32 func_02012be4(WifiList *wifiList);
void *func_02012908(int num, HeapID heapId);
void func_02012994(void *work);
void func_02012a4c(void);
void GFL_NetErrAbort(void);
void func_02012050(void);
// Shows the error screen for an assertion that failed
void AssertFailErrorDisp(void);
// The wireless signal level of the DS Download Play, and the record of the best level a scan saw, which it sets and
// clears
u16 func_02012ea0(void);
void func_02012ebc(u8 linkLevel);
void func_02012edc(void);

// The device table for a GFL_NET_TYPE_*, from outside the library
const GFLNetDevTable *func_020116c0(int type);
void func_02011778(int type);
// GFL_SndBGMSetVolume, unless a connection is up and overlay 11's func_ov011_02151dec returns TRUE
void func_02011bdc(u16 trackMask, s32 volume);

// net.c: starts and ends the network, and passes calls to the device and the other parts of the library
void func_020425a0(int a0, int a1, HeapID parentHeapId, HeapID heapId);
void func_020425ec(const GFLNetInitData *pNetInit, void (*callback)(void *work), void *work);
BOOL func_02042788(void);
// Whether the network has ended
BOOL func_020427a4(void);
// Ends the network, calling back with the game's work once it has
BOOL func_02042860(void (*callback)(void *work));
void func_020428a0(void);
void *func_020428a8(int index);
u8 *func_020428c8(int index);
// Steps the network while the game waits for it, as before a soft reset
BOOL func_020428e0(void);
void func_02042918(void);
void func_02042950(const u8 *mac);
void func_0204295c(const u8 *mac);
void func_02042968(void);
void func_02042970(void);
void func_020429a8(void (*callback)(void *work), void (*a1)(void *work, BOOL a1), void (*a2)(void *work));
void func_020429d8(int a0);
void func_020429e0(u8 *mac, int index);
void func_020429e8(void *a0);
void func_020429f0(void);
void func_020429f8(void (*callback)(void *work));
// The argument is unused
void func_02042a10(int unused);
void func_02042a1c(int a0);
void func_02042a30(int mode, int a1, const u8 *mac);
BOOL func_02042a38(void);
void func_02042a40(int a0);
u8 func_02042a48(void);
void func_02042a50(int a0);
u8 func_02042a6c(NetHandle *handle);
int func_02042a78(void);
BOOL func_02042a80(int netId);
void func_02042a9c(NetHandle *handle, int a1);
BOOL func_02042ab8(void);
// Whether the network is infrared, or Wi-Fi
BOOL func_02042b00(void);
BOOL func_02042b20(void);
// Calls into the functions that show the wireless strength icons
void func_02042ba8(BOOL top, HeapID heapId);
BOOL func_02042bc4(void);
// Whether a machine's negotiation has been accepted
BOOL func_02042bd8(NetHandle *handle);
// Send a command to every machine, or to one; the game's commands wait for the negotiation
BOOL func_02042be8(NetHandle *handle, int command, u16 size, const void *data);
BOOL func_02042c18(NetHandle *handle, u8 sendID, u16 command, u32 size, const void *data, u32 a5, BOOL a6, BOOL noCopy);
BOOL func_02042c9c(NetHandle *handle, int dest, u16 command, int size, const void *data, u32 a5, BOOL a6, BOOL noCopy);
BOOL func_02042cfc(NetHandle *handle);
void func_02042d04(NetHandle *handle, u16 timing);
BOOL func_02042d0c(NetHandle *handle, u16 timing);
// The second argument is unused
void func_02042d14(u8 gameCommandBase, u8 unused);
u8 func_02042d34(void);
// Whether the net game command base is one of the battle kinds
BOOL func_02011844(void);
int func_02042d48(void);
int func_02042d64(void);
u8 func_02042d80(void);
void func_02042d8c(BOOL a0);
// The game's work, which the callbacks get
void *func_02042d94(void);
void func_02042dac(void *work);
// The number of machines, and the size of each machine's data in a packet
int func_02042dc0(void);
int func_02042de8(void);
u16 func_02042df4(void);
void func_02042e18(void);
void func_02042e30(int a0);
void func_02042e54(int a0);
GFLNetSys *func_02042e78(void);
GFLNetInitData *func_02042e84(void);
void func_02042e94(u8 a0);
void func_02042e9c(BOOL a0);
void *allocConfigDSSoftwareFeature(HeapID heapId, u32 size, const char *file, u16 line);
void func_02042ed0(void *ptr);
// Record and read back the offset of BG 1 of the main engine, which bg_sys.c sets whenever it changes
void func_02042ee0(int x, int y);
void func_02042eec(int *x, int *y);
void func_02042efc(const GFLNetInitData *pNetInit);
void func_02042f10(void);
BOOL func_02042f24(void);
void func_02042f2c(int x, int y);
void func_02042f40(void);
void func_02042f50(BOOL a0);

#endif // POKEBW2_GFL_NET_H
