// xsettings.h
// @author octopoulos
// @version 2025-07-23

#pragma once

#include "engine-settings.h"

struct XSettings : public EngineSettings
{
	// [analysis]
	// 0
	str256 appId;  ///< appId to synchronize with the browser
	int    gameId; ///< 0: gori, 1: custom
	int    view;   ///< &1: analysis, &2: text, &4: division, &8: base, &16: border, &32; coord

	// [ui]
	str512 recentFiles[6]; ///

	// [user]
	str256 userEmail; ///< login
	str256 userHost;  ///< ws(s) url
	str256 userPw;    ///< password
};

#include "game-settings.h"
