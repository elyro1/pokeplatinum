#ifndef BWBATTLE_SHIM_BW_SHIM_H
#define BWBATTLE_SHIM_BW_SHIM_H

// Shared by the shims (src/bwbattle/shim), which implement what the BW2 view calls outside the files bwbattle copies

void BwShim_Log(const char *format, ...);
// The host's (bw_view_host.c): a trainer was added at the view position, by the host or by an effect
void BwView_OnTrainerAdded(u32 viewPos);

// Marks a shim that doesn't do what BW2's function does yet, logging the first call
#define BW_SHIM_TODO()                                             \
    do {                                                           \
        static int sLogged;                                        \
        if (!sLogged) {                                            \
            sLogged = 1;                                           \
            BwShim_Log("not implemented yet: %s", __func__);       \
        }                                                          \
    } while (0)

#endif // BWBATTLE_SHIM_BW_SHIM_H
