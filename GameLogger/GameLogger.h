#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include <map>
#include <string>

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

//main player stats
typedef struct {
    std::string team;
    std::string name;
    std::string club;
    int mmr;
    int score;
    int goals;
    int ownGoals;
    int assists;
    int saves;
    int shots;
    int demos;
    int damage;
    int mvp;
} PlayerAndMatchStats;

//
class GameLogger : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow {

public:

    //
	void onLoad() override;
	void onUnload() override;

    //
    void RenderSettings() override;
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;

private:
    
    //event hooks
    void onMatchEnded(std::string eventName);

    //
    std::string getTimestamp(std::string format);
    void updatePlayerStats();
 
};
