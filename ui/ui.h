// ui.h
// @author octopoulos
// @version 2025-07-31

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
bool          AddCombo(const std::string& name, const char* text);
bool          AddCombo(const std::string& name, const char* text, const char* texts[], const VEC_INT values);
bool          AddDragFloat(const std::string& name, const char* text, float speed = 0.0005f, const char* format = "%.3f");
bool          AddDragInt(const std::string& name, const char* text, float speed = 1.0f, const char* format = "%d");
bool          AddSliderBool(const std::string& name, const char* text, const char* format = "%d", bool vertical = false, const ImVec2& size = { 30, 120 });
bool          AddSliderFloat(const std::string& name, const char* text, const char* format = "%.3f");

/// Create an horizontal or vertical slider
/// @param format: nullptr => slider enum, to use instead of AddCombo
bool AddSliderInt(const std::string& name, const char* text, const char* format = "%d", bool vertical = false, const ImVec2& size = { 30, 120 }, bool isBool = false);

bool AddSliderInt(const std::string& name, const char* text, int* value, int count, int min, int max, const char* format = "%d");

void AddSpace(float height = -1.0f);

/// Right click => reset to default
bool ItemEvent(const std::string& name, int index = -1);

/// Show a 2 column Stats table
void ShowTable(const std::vector<std::tuple<std::string, std::string>>& stats);

uint32_t LoadTexture(const std::filesystem::path& path, std::string name);
uint32_t LoadTexture(const uint8_t* data, uint32_t size, std::string name);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONTROLS
///////////

bool AddMenu(const char* text, const char* shortcut, CommonWindow& window);

/// Image text button aligned on a row
int DrawControlButton(uint32_t texId, const ImVec4& color, std::string name, const char* label, int textButton, float uiScale);

/// Draw all UI
void DrawWindows(const std::vector<CommonWindow*>& windows, bool showImGuiDemo);

/// Set alpha for the next window
bool SetAlpha(float alpha);

/// Hide/unhide windows:
/// - only change hidden, not isOpen
bool ShowWindows(const std::vector<CommonWindow*>& windows, bool show, bool force);

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
// SETTINGS
///////////

CommonWindow& GetEnSettingsWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THEME
////////

CommonWindow& GetThemeWindow();
ImFont*       FindFont(const std::string& name);
void          UpdateFonts();
void          UpdateTheme();

} // namespace ui
