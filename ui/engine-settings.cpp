// engine-settings.cpp
// @author octopoulo
// @version 2025-07-26

#include "stdafx.h"
#include "ui/engine-settings.h"
//
#include "AI/common.h" // Config

static std::string settingsToml = "engine.ini";

extern const char* sFullScreens[3];

const char* sAspectRatios[] = { "1:1", "4:3", "3:2", "16:9", "Native", "Window" };
const char* sProjections[]  = { "Orthogonal", "Perspective" };
const char* sRenderModes[]  = { "None", "Screen", "Model", "Screen + Model" };
const char* sThemes[]       = { "Classic", "Custom", "Dark", "Light", "Xemu" };
const char* sVSyncs[]       = { "Off", "On", "Adaptive" };

// global variable
EngineSettings  engineSettings;
EngineSettings* appSettings = &engineSettings;

// MAPPING
//////////

#define XSETTINGS EngineSettings

static std::vector<Config> configs = {
	#include "game-config.inl"
};

static std::unordered_map<std::string, Config*> configMap;

// API
//////

void InitEngineSettings()
{
	InitConfig(configs, settingsToml);
	InitSettings(appSettings, sizeof(EngineSettings), false);
}

void LoadEngineSettings(std::string baseName, std::string_view suffix)
{
	LoadSettings(appSettings, sizeof(EngineSettings), baseName, suffix);
}

int SaveEngineSettings(std::string baseName, std::string_view suffix, const USET_STR& sections)
{
	SaveSettings(baseName, suffix, sections);
	return 1;
}
