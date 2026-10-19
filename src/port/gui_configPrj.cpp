#include <nitro.h>
#include <simulator/sim.h>
#include <simulator/sim_gui.hpp>
#include <simulator/imgui/imgui.hpp>

#include "sim_gui_prj.hpp"

#include "port/sim_config_prj.h"

#include "bwbattle/bwbattle.h"

#include <string>

namespace SIM::GUI {

static void ConfigCheckBox(const char *label, BOOL *configVar, bool &configChanged)
{
    bool checked = *configVar != FALSE;
    if (ImGui::Checkbox(label, &checked)) {
        *configVar = checked ? TRUE : FALSE;
        configChanged = true;
    }
}

static std::string sBw2ExtractMessage;

// Black 2 / White 2's battle view (docs/bwbattle.md): the setting, and the extraction of its assets from the player's
// own ROM
static void Bw2BattleVisualsConfig(SIM_Config_prj_type *myConfig, bool &configChanged)
{
    bool ready = BwBattle_AssetsReady();

    ImGui::BeginDisabled(!ready);
    ConfigCheckBox("BW2 battle visuals", &myConfig->bw2BattleVisuals, configChanged);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Show battles with Pokemon Black 2 / White 2's 3D arena and animated sprites.\n"
            "Needs graphics from your own US Black 2 or White 2 ROM. Takes effect from the next battle.");
    }
    ImGui::SameLine();
    if (ImGui::Button(ready ? "Re-extract BW2 ROM..." : "Select BW2 ROM...")) {
        OpenFileDialog([](std::string path) {
            sBw2ExtractMessage = BwBattle_ExtractResultMessage(BwBattle_ExtractAssets(path.c_str()));
        });
    }
    if (!sBw2ExtractMessage.empty()) {
        ImGui::TextUnformatted(sBw2ExtractMessage.c_str());
    } else if (!ready) {
        ImGui::TextUnformatted("BW2 battle graphics not extracted.");
    }
}

void ConfigPrjMain(bool * openState) {
    ImGui::Begin("pokeplatinum Config", openState);
    SIM_Config_prj_type * myConfig = SIM_Config_prj_GetConfig();
    bool configChanged = false;

    int frameRate = myConfig->enable60fps ? 1 : 0;
    if (ImGui::Combo("Frame rate", &frameRate, "30 FPS\0" "60 FPS\0")) {
        myConfig->enable60fps = frameRate == 1;
        configChanged = true;
    }
    ImGui::BeginDisabled(!myConfig->enable60fps);
    int pacing = myConfig->battlePacing;
    if (ImGui::Combo("Battle pacing", &pacing, "Original\0Smooth\0Quick\0")) {
        myConfig->battlePacing = static_cast<BattlePacing>(pacing);
        configChanged = true;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Original: original animation timing.\n"
            "Smooth: original animation timing with sprite interpolation.\n"
            "Quick: faster battle animations.\n"
            "UI timing is controlled separately. At 30 FPS all presets use original timing.\n"
            "Interpolation adds up to one animation tick of visual delay.");
    }
    ConfigCheckBox("Normal Battle UI & Timing", &myConfig->battleUiSpeedFix, configChanged);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Keep HP/EXP bars, messages and battle UI delays at normal speed at 60 FPS, independently of Battle pacing.");
    }
    ImGui::EndDisabled();
    Bw2BattleVisualsConfig(myConfig, configChanged);
    ConfigCheckBox("Enable GF_Asserts", &myConfig->enableAsserts, configChanged);
    ConfigCheckBox("DebugBreak on GF assertion fail", &myConfig->breakDebuggerOnGfAssert, configChanged);

    if(configChanged) {
        SIM_Config_prj_SaveConfigFile(myConfig);
    }


    ImGui::End();
}
}
