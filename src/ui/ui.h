// ui.h
// @author octopoulos
// @version 2025-08-02

#pragma once

#include "imgui-include.h"

namespace ui
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COMMON
/////////

#define CHECK_DRAW() \
	if (!isOpen || (hidden & 1)) return

#define ESHOW_TREE(flag) ((appSettings->tree & flag) ? ImGuiTreeNodeFlags_DefaultOpen : 0)
#define SHOW_TREE(flag)  ((xsettings.tree & flag) ? ImGuiTreeNodeFlags_DefaultOpen : 0)

class CommonWindow
{
public:
	float       alpha  = 1.0f;  ///
	int         drawn  = 0;     ///
	int         focus  = 0;     ///
	int         hidden = 0;     ///< automatic flag, 2=cannot be hidden
	bool        isOpen = false; ///< manual flag, the user opened/closed the window
	std::string name;           ///

	virtual ~CommonWindow() {}
	virtual void Draw() {}
};

CommonWindow& GetCommonWindow();

/// Add a combo with label left/right + special Blender support
bool AddCombo(const std::string& name, const char* label);

/// Add a combo with label left/right + special Blender support
bool AddCombo(const std::string& name, const char* label, const char* texts[], const VEC_INT values);

/// Add a drag float with label left/right + special Blender support
bool AddDragFloat(const std::string& name, const char* text, float speed = 0.0005f, const char* format = "%.3f");

/// Add a drag int with label left/right + special Blender support
bool AddDragInt(const std::string& name, const char* text, float speed = 1.0f, const char* format = "%d");

/// Add an input text input with label left/right + special Blender support
void AddInputText(const std::string& name, const char* label, size_t size = 256, int flags = 0);

/// Add a slider bool with label left/right + special Blender support
bool AddSliderBool(const std::string& name, const char* text, const char* format = "%d", bool vertical = false, const ImVec2& size = { 30, 120 });

/// Add a slider float with label left/right + special Blender support
bool AddSliderFloat(const std::string& name, const char* text, const char* format = "%.3f");

/// Create an horizontal or vertical slider, with label left/right + special Blender support
/// @param format: nullptr => slider enum, to use instead of AddCombo
bool AddSliderInt(const std::string& name, const char* text, const char* format = "%d", bool vertical = false, const ImVec2& size = { 30, 120 }, bool isBool = false);

/// Create an horizontal or vertical slider, with label left/right + special Blender support
/// @param format: nullptr => slider enum, to use instead of AddCombo
bool AddSliderInt(const std::string& name, const char* text, int* value, int count, int min, int max, const char* format = "%d");

void AddSpace(float height = -1.0f);

/// Right click => reset to default
bool ItemEvent(const std::string& name, int index = -1);

/// Show a 2 column Stats table
void ShowTable(const std::vector<std::tuple<std::string, std::string>>& stats);

//uint32_t LoadTexture(const std::filesystem::path& path, std::string name);
//uint32_t LoadTexture(const uint8_t* data, uint32_t size, std::string name);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONTROLS
///////////

CommonWindow& GetControlsWindow();

/// Image text button aligned on a row
int DrawControlButton(uint32_t texId, const ImVec4& color, std::string name, const char* label, int textButton, float uiScale);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LOG
//////

CommonWindow& GetLogWindow();

/// Add Log - std::string version
/// @param id: 0: log, 1: error, 2: info, 3: warning
void AddLog(int id, const std::string& text);

/// Clear part_of/all the log
void ClearLog(int id);

/// Erase the previous x lines
void RemovePrevious(std::string_view prevText);

/// Indent/unindent log
void Tab(int value);

#undef Log
#undef LogError
#undef LogInfo
#undef LogOutput
#undef LogWarning

#define Log(...)        AddLog(0, fmt::format(__VA_ARGS__))
#define LogError(...)   AddLog(1, fmt::format(__VA_ARGS__))
#define LogInfo(...)    AddLog(2, fmt::format(__VA_ARGS__))
#define LogOutput(...)  AddLog(4, fmt::format(__VA_ARGS__))
#define LogWarning(...) AddLog(3, fmt::format(__VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MENU
///////

/// Current menu height
float GetMenuHeight();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SETTINGS
///////////

CommonWindow& GetSettingsWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THEME
////////

CommonWindow& GetThemeWindow();
ImFont*       FindFont(const std::string& name);
void          UpdateFonts();
void          UpdateTheme();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI
/////

/// Add a menu item for a window toggle
bool AddMenu(const char* text, const char* shortcut, CommonWindow& window);

/// Draw all UI
void DrawWindows();

/// Create a list of the windows
void ListWindows();

/// Set alpha for the next window
bool SetAlpha(float alpha);

/// Show the menu bar
void ShowMainMenu(float alpha);

/// Hide/unhide windows
///  - only change hidden, not isOpen
bool ShowWindows(bool show, bool force);

} // namespace ui
