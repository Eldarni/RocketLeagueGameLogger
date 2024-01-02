
//
#include "pch.h"

//
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

//
#include "GameLogger.h"

//
BAKKESMOD_PLUGIN(GameLogger, "Log game stats to CSV", plugin_version, PLUGINTYPE_FREEPLAY)

//
std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

//prepare a map for the player stats
std::map<std::string, PlayerAndMatchStats> playerStats;

//Set up
void GameLogger::onLoad() {

    //
	_globalCvarManager = cvarManager;

    //
    cvarManager->registerCvar("gamelogger_filename", gameWrapper->GetBakkesModPath().string() + "\\game-logger.csv", "Where should we save the file to. Use \"\\\" as path seperator", true, false, 0, false, 0, true);

    //update the player stats every kickoff - should deal with most issues around players leaving (won't be perfect but the only other option will be per tick)
    gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", [this](std::string eventName) {
        updatePlayerStats();
    });

	//main hook to write the stats to the csv - called at the end of the game - so if the player running the plugin rage quits
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchEnded", std::bind(&GameLogger::onMatchEnded, this, std::placeholders::_1));

}

//Tidy up
void GameLogger::onUnload() {
    gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.StartRound");
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchEnded");
}

//Write the stats to the csv
void GameLogger::onMatchEnded(std::string eventName) {

    //get the RL ServerWrapper - this gets us access to the stats
    ServerWrapper sw = gameWrapper->GetOnlineGame();
    if (sw.IsNull()) {
        cvarManager->log("ServerWrapper is null. CSV cannot be recorded");
        return;
    }

    //create a string stream
    std::stringstream ss;   

    //get the current time stamp
    std::string timestamp = getTimestamp("%Y-%m-%d %H:%M:%S");

    //get the game mode e.g. Dual or Doubles
    std::string gameMode = sw.GetPlaylist().GetTitle().ToString() + ((sw.GetPlaylist().GetbRanked()) ? " (Ranked)" : "");
    if (gameMode == "") {
        gameMode = "Unknown";
    }

    //
    std::fstream csv;   

    //
    std::string filename = cvarManager->getCvar("gamelogger_filename").getStringValue();

    //
    if (!std::filesystem::exists(filename)) {

        //
        std::filesystem::create_directories(filename.substr(0, filename.rfind("\\")));

        //
        csv.open(filename, std::fstream::out);
        csv << "Timestamp,Game mode,Team,Player,Club,MMR,Score,Goals,Own Goals,Assists,Saves,Shots,Demos,Damage,MVP\n";
        csv.close();

    }

    //
    csv.open(filename, std::fstream::out | std::fstream::app);
    
    //update the player stats one last time
    updatePlayerStats();

    //
    for (auto const& [playerID, playerStats] : playerStats) {
        
        //
        ss << timestamp << "," << gameMode << "," << playerStats.team << "," << playerStats.name << "," << playerStats.club << "," << playerStats.mmr << "," << playerStats.score << "," << playerStats.goals << "," << playerStats.ownGoals << "," << playerStats.assists << "," << playerStats.saves << "," << playerStats.shots << "," << playerStats.demos << "," << playerStats.damage << "," << playerStats.mvp << "\n";
        csv << ss.str();
        
        //empty the string
        ss.str(std::string());

    }

    //
    csv.close();

    //clear the map between rounds
    playerStats.clear();

}

//Gets the current date/time as a formatted string
std::string GameLogger::getTimestamp(std::string format) {

    //
    const std::time_t now = std::time(nullptr);
    const std::tm currTimeLocal = *std::localtime(&now);

    //
    std::stringstream ss;
    ss << std::put_time(&currTimeLocal, format.c_str());

    //
    return ss.str();

}

//update the player stats map
void GameLogger::updatePlayerStats() {

    //get the RL ServerWrapper - this gets us access to the stats
    ServerWrapper sw = gameWrapper->GetOnlineGame();
    if (sw.IsNull()) {
        cvarManager->log("ServerWrapper is null. Error getting player stats");
        return;
    }

    //get a map of all the stats for each player - should i null check these - prob
    ArrayWrapper<TeamWrapper> teams = sw.GetTeams();
    ArrayWrapper<PriWrapper>  players = sw.GetPRIs();

    //
    MMRWrapper mw = gameWrapper->GetMMRWrapper();

    //
    for (int i = 0; i < players.Count(); i++) {
        
        //
        PriWrapper player = players.Get(i);

        //
        std::string playerID = player.GetUniqueIdWrapper().GetIdString();

        //
        std::string team = ((player.GetTeamNum() == 0) ? "Blue" : "Orange");

        //get the players name
        std::string name = player.GetPlayerName().ToString();

        //get the club name (formatted as [tag] name)
        ClubDetailsWrapper clubDetails = player.GetClubDetails();
        std::string club = ((clubDetails.IsNull() == false) ? "[" + clubDetails.GetClubTag().ToString() + "] " + clubDetails.GetClubName().ToString() : "");

        //calculate the mmr for each player
        int mmr = 0;
        int currentPlaylist = mw.GetCurrentPlaylist();
        if (mw.IsSynced(player.GetUniqueIdWrapper(), currentPlaylist)) {
            mmr = std::round(mw.GetPlayerMMR(player.GetUniqueIdWrapper(), currentPlaylist));
        }

        //now for how well they did in the match
        int score    = player.GetMatchScore();
        int goals    = player.GetMatchGoals();
        int ownGoals = player.GetMatchOwnGoals();
        int assists  = player.GetMatchAssists();
        int saves    = player.GetMatchSaves();
        int shots    = player.GetMatchShots();
        int demos    = player.GetMatchDemolishes();
        int damage   = player.GetMatchBreakoutDamage();
        int mvp      = player.GetbMatchMVP();

        //
        playerStats[playerID] = PlayerAndMatchStats{ team, name, club, mmr, score, goals, ownGoals, assists, saves, shots, demos, damage, mvp };

    }

}