#ifndef POKEPLATINUM_PC_FRAME_PROFILE_H
#define POKEPLATINUM_PC_FRAME_PROFILE_H
#if defined(SDK_BUILD_LINUX) || defined(SDK_BUILD_WIN64)
#include <simulator/sim_frame_profile.h>
#define PC_PROFILE_BEGIN(name)        u64 name = SIM_ProfileNow()
#define PC_PROFILE_END(name, counter) SIM_ProfileAdd(counter, SIM_ProfileNow() - name)
#else
#define PC_PROFILE_BEGIN(name)
#define PC_PROFILE_END(name, counter)
#endif
#endif
