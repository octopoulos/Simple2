// xsettings.h
// @author octopoulos
// @version 2025-07-24

#pragma once

#include "engine-settings.h"

struct XSettings : public EngineSettings
{
	// [analysis]
	// 0
	str256 appId;  ///< appId to synchronize with the browser
	int    gameId; ///< 0: gori, 1: custom

	// [physics]
	bool physPaused; ///< paused physics

	// [render]
	bool  instancing; ///< use mesh instancing
	float orthoZoom;  ///< zoom in orthographic projection

	// [ui]
	bool   nvidiaEnc;      ///< use nVidia encoding
	str512 recentFiles[6]; ///< recent files for quick load
	bool   videoCapture;   ///< allow video capture

	// [user]
	str256 userEmail; ///< login
	str256 userHost;  ///< ws(s) url
	str256 userPw;    ///< password
};

#include "game-settings.h"
