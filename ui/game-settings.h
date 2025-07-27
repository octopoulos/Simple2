// game-settings.inc.h
// @author octopoulo
// @version 2025-07-23
// - include at the end of xsettings.h

#pragma once

extern XSettings xsettings;

/// Populate the settingsMap + find the settings folder
void InitGameSettings();

/// Load settings.ini
void LoadGameSettings(int gameId = -1, std::string baseName = "", std::string_view suffix = "");

/// Save settings.ini
/// @param sections: empty to save all
int SaveGameSettings(std::string baseName = "", bool saveGame = true, std::string_view suffix = "", const USET_STR& sections = {});

/// Get the settings folder (C++)
std::filesystem::path GameFolder();

/// Get the game string
/// @param gameId: -1: get current gameId
/// @returns "x" if incorrect gameId
const char* GameName(int gameId = -1);
