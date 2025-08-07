// game-settings.inc.cpp
// @author octopoulo
// @version 2025-07-23
// - include at the end of xsettings.cpp

#ifdef XSETTINGS

#	ifdef WITH_SDL3
#		include <SDL3/SDL_filesystem.h>
#	endif // WITH_SDL3

constexpr int NUM_GAMES = int(sizeof(sGames) / sizeof(*sGames));

static XSettings gameSettings[NUM_GAMES];
static bool      gameSettingsLoaded[NUM_GAMES];

// API
//////

std::filesystem::path GameFolder() { return DataFolder() / GameName(); }

const char* GameName(int gameId)
{
	if (gameId == -1) gameId = xsettings.gameId;
	return (gameId >= 0 && gameId < NUM_GAMES) ? sGames[gameId] : "x";
}

void InitGameSettings()
{
	InitConfig(configs, settingsToml);
	{
		std::string exePath;
#	ifdef WITH_SDL3
		exePath = SDL_GetBasePath();
		if (exePath.ends_with('/') || exePath.ends_with('\\')) exePath.pop_back();
		if (DEV_path) ui::Log("InitGameSettings: {} : {}", std::filesystem::current_path(), exePath);
#	endif // WITH_SDL3
		InitSettings(&xsettings, sizeof(XSettings), true, exePath);
	}

	// load game settings
	for (int gameId = 0; gameId < NUM_GAMES; ++gameId)
	{
		memset(&xsettings, 0, sizeof(XSettings));
		DefaultSettings(&xsettings, sizeof(XSettings), nullptr);
		LoadGameSettings(gameId, "", "-def");
		memcpy(&gameSettings[gameId], &xsettings, sizeof(XSettings));
		gameSettingsLoaded[gameId] = true;
	}

	// update engine
	appSettings = &xsettings;
}

void LoadGameSettings(int gameId, std::string baseName, std::string_view suffix)
{
	if (baseName.empty() && gameId >= 0 && gameId < NUM_GAMES)
		baseName = fmt::format("{}{}.ini", sGames[gameId], suffix);

	LoadSettings(&xsettings, sizeof(XSettings), baseName, suffix);

	// game settings
	if (gameId >= 0 && gameId < NUM_GAMES)
		xsettings.gameId = gameId;

	xsettings.windowSize[0] = (std::max(32, xsettings.windowSize[0]) + 7) & ~15;
	xsettings.windowSize[1] = (std::max(32, xsettings.windowSize[1]) + 3) & ~7;
}

int SaveGameSettings(std::string baseName, bool saveGame, std::string_view suffix, const USET_STR& sections)
{
	SaveSettings(baseName, suffix, sections);

	// save HD/Omega.ini as well - only analysis section
	if (saveGame && sections.empty())
	{
		if (const auto gameId = xsettings.gameId; gameId >= 0 && gameId < NUM_GAMES)
			SaveSettings(fmt::format("{}{}.ini", sGames[gameId], suffix), "", { "analysis", "capture", "ocr", "user" });
	}
	return 1;
}

#	undef XSETTINGS
#endif // XSETTINGS
