#ifndef BWBATTLE_ASSETS_BW2_ASSETS_H
#define BWBATTLE_ASSETS_BW2_ASSETS_H

#include "types.h"

// The archives extracted from the BW2 ROM (src/bwbattle/assets/bw2_assets.c). The extraction itself is in bwbattle.h

// Points BW2's archive system (gfl/arc.h) at the extracted archives
void BwAssets_InitArcSys(void);
BOOL BwAssets_ArcExists(u32 arcId);

#endif // BWBATTLE_ASSETS_BW2_ASSETS_H
