// xsettings.h
// @author octopoulos
// @version 2025-07-30

#pragma once

#include "ui/EngineSettings.h"

struct XSettings : public EngineSettings
{
	// [analysis]
	// 0
	str256 appId;  ///< appId to synchronize with the browser
	int    gameId; ///< 0: gori, 1: custom

	// [physics]
	bool bulletDebug; ///< bullet debug draw
	bool physPaused;  ///< paused physics

	// [render]
	float distance;   ///< distance between camera and cursor
	bool  instancing; ///< use mesh instancing
	float orthoZoom;  ///< zoom in orthographic projection

	// [ui]
	float cameraSpeed;    ///< camera movement speed
	bool  nvidiaEnc;      ///< use nVidia encoding
	str2k recentFiles[6]; ///< recent files for quick load
	bool  videoCapture;   ///< allow video capture
	float zoomKb;         ///< zoom speed with the keyboard
	float zoomWheel;      ///< zoom speed with the mouse wheel

	// [user]
	str256 userEmail; ///< login
	str256 userHost;  ///< ws(s) url
	str256 userPw;    ///< password
};

#include "game-settings.h"
