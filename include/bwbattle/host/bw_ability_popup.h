#ifndef BWBATTLE_HOST_BW_ABILITY_POPUP_H
#define BWBATTLE_HOST_BW_ABILITY_POPUP_H

#include "types.h"

void BwAbilityPopup_Init(void);
void BwAbilityPopup_Delete(void);
BOOL BwAbilityPopup_Show(unsigned int side, const u16 *nickname, const u16 *ability);
BOOL BwAbilityPopup_IsBusy(void);
void BwAbilityPopup_SetHeld(unsigned int side, BOOL held);
void BwAbilityPopup_Step(void);
void BwAbilityPopup_Draw(void);

#endif
