#include <nitro.h>
#include <simulator/imgui/imgui.hpp>
#include <simulator/sim.h>
#include <simulator/sim_gui.hpp>

#include "generated/poketch_apps.h"

#include "port/debug_field.h"
#include "port/sim_config_prj.h"

#include "sim_gui_prj.hpp"

namespace SIM::GUI {

static void ConfigCheckBox(const char *label, bool *configVar, bool &configChanged)
{
    if (ImGui::Checkbox(label, configVar)) {
        configChanged = true;
    }
}

void CheatsMain(bool *openState)
{
    ImGui::Begin("Cheats", openState);
    SIM_Config_prj_type *myConfig = SIM_Config_prj_GetConfig();
    bool configChanged = false;

    ConfigCheckBox("Walk Through Walls", (bool *)&myConfig->walkThroughWalls, configChanged);
    ConfigCheckBox("Disable Random Encounters", (bool *)&myConfig->disableRandomEncounters, configChanged);
    ConfigCheckBox("Run from Trainer Battles", (bool *)&myConfig->runFromTrainerBattles, configChanged);

    ImGui::SeparatorText("Poketch");
    int appCount = DEBUG_GetPoketchAppCount();
    ImGui::BeginDisabled(appCount < 0);
    if (ImGui::Button("Unlock all Poketch apps")) {
        DEBUG_UnlockAllPoketchApps();
        appCount = DEBUG_GetPoketchAppCount();
    }
    ImGui::EndDisabled();
    if (appCount >= 0) {
        ImGui::Text("Registered apps: %d / %d", appCount, POKETCH_APPID_MAX);
        ImGui::TextUnformatted("Use L/R in the Poketch. Save in-game to keep the unlocks.");
    } else {
        ImGui::TextUnformatted("Load a game and acquire the Poketch first.");
    }

    if (configChanged) {
        SIM_Config_prj_SaveConfigFile(myConfig);
    }

    ImGui::End();
}
}
