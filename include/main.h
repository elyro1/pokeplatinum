#ifndef POKEPLATINUM_MAIN_H
#define POKEPLATINUM_MAIN_H

#include <nitro/fs.h>

#include "overlay_manager.h"
#include "savedata.h"

enum OSResetParameter {
    RESET_CLEAN = 0,
    RESET_ERROR,
};

typedef struct ApplicationArgs {
    int unk_00;
    BOOL error;
    SaveData *saveData;
} ApplicationArgs;

void EnqueueApplication(FSOverlayID overlayID, const ApplicationManagerTemplate *template);
void InitRNG(void);
void HandleConsoleFold(void);

#ifdef SDK_PORT
// Keeps the main loop at the original 30 Hz even when 60 FPS is enabled, for
// sequences whose frame counts are timed against real-time audio.
void SetForce30Hz(BOOL force);
#else
static inline void SetForce30Hz(BOOL force)
{
    (void)force;
}
#endif

#endif // POKEPLATINUM_MAIN_H
