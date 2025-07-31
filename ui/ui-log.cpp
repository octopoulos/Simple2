// ui-log.cpp
// @author octopoulos
// @version 2025-07-26

#include "stdafx.h"
#include "ui/ui.h"

#define LOG_CONSOLE

namespace ui
{

// clang-format off
const ImVec4 colorValues[] = {
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // text
	{ 1.0f, 0.5f, 0.5f, 1.0f }, // error
	{ 0.3f, 0.7f, 1.0f, 1.0f }, // info
	{ 1.0f, 0.8f, 0.5f, 1.0f }, // warning
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // output
};
// clang-format on

std::mutex logMutex;

struct LogEntry
{
	int         color;
	std::string date;
	std::string text;

	LogEntry(int color, const std::string& date, const std::string& text)
	    : color(color)
	    , date(date)
	    , text(text)
	{
	}
	LogEntry(int color, std::string&& date, std::string&& text)
	    : color(color)
	    , date(std::move(date))
	    , text(std::move(text))
	{
	}
};

class LogWindow : public CommonWindow
{
public:
	LogWindow()
	{
		name   = "Log";
		isOpen = true;
	}

	void AddLog(int id, const std::string& text)
	{
		std::lock_guard<std::mutex> lock(logMutex);

		std::istringstream iss(text);
		std::string        part;
		while (std::getline(iss, part, '\n'))
		{
			const std::string result = tab + part;
#ifdef LOG_CONSOLE
			fwrite(result.data(), 1, result.size(), stderr);
			fputc('\n', stderr);
#endif // LOG_CONSOLE

			if (offset && id < 4)
				colorLines[0].emplace_back(id, "", result);

			if (int colorOffset = id + offset; colorOffset >= 0 && colorOffset < TO_INT(colorNames.size()))
				colorLines[colorOffset].emplace_back(offset ? id : 0, "", result);
		}
	}

	void ClearLog(int id)
	{
		std::lock_guard<std::mutex> lock(logMutex);

		if (id == -1)
		{
			for (size_t i = 0; i < colorNames.size(); ++i)
				colorLines[i].clear();
		}
		else colorLines[id + offset].clear();
	}

	void Draw()
	{
		CHECK_DRAW();
		if (!ImGui::Begin(name.c_str(), &isOpen, ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::End();
			return;
		}

		const auto numTab = colorNames.size();
		if (numTab > 1)
			if (ImGui::BeginTabBar("Log#tabs"))
			{
				for (int i = 0; i < numTab; ++i)
					if (ImGui::BeginTabItem(colorNames[i].c_str()))
					{
						active = i - offset;
						ImGui::EndTabItem();
					}
				ImGui::EndTabBar();
			}

		const auto& lines  = colorLines[active + offset];
		const auto  region = ImGui::GetContentRegionAvail();
		ImVec2      childDims(region.x, region.y);

		ImGui::BeginChild(fmt::format("Scroll{}", active).c_str(), childDims, false);
		{
			ImGui::PushFont(FindFont("mono"));
			ImGuiListClipper clipper;
			clipper.Begin(TO_INT(lines.size()));
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
				{
					auto color = lines[i].color;
					if (color) ImGui::PushStyleColor(ImGuiCol_Text, colorValues[color]);
					ImGui::TextUnformatted(lines[i].text.c_str());
					if (color) ImGui::PopStyleColor();
				}
			}
			clipper.End();
			ImGui::PopFont();

			// auto scroll
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
		ImGui::End();
	}

	void RemovePrevious(std::string_view prevText)
	{
		if (prevText.empty()) return;
		std::lock_guard<std::mutex> lock(logMutex);

		// try to remove from non-All tabs
		bool found = false;
		for (int i = 1, numTab = TO_INT(colorNames.size()); i < numTab; ++i)
			if (const auto size = colorLines[i].size(); size > 0 && colorLines[i][size - 1].text == prevText)
			{
				const auto prevSize = colorLines[i].size();
				colorLines[i].pop_back();
				found = true;
			}

		// remove from All tab
		if (found)
		{
			auto& lines = colorLines[0];
			for (auto it = lines.end(); it-- != lines.begin();)
				if (it->text == prevText)
				{
					const auto prevSize = lines.size();
					lines.erase(it);
					break;
				}
		}
	}

	void Tab(int value)
	{
		if (value > 0)
			tab += "  ";
		else if (tab.size() >= 2)
			tab = tab.substr(0, tab.size() - 2);
	}

protected:
	int                   active        = -1;
	std::vector<LogEntry> colorLines[6] = {};
	VEC_STR               colorNames    = { "All", "Text", "Error", "Info", "Warning", "Output" };
	int                   offset        = 1;
	std::string           tab           = "";
};

static LogWindow logWindow;
CommonWindow&    GetLogWindow() { return logWindow; }

// API
//////

/// Add Log - va_list version
/// @param id: 0: log, 1: error, 2: info, 3: warning
static void AddLogV(int id, const char* fmt, va_list args)
{
	const int   bufSize = 2048;
	static char buf[bufSize];

	int w = stbsp_vsnprintf(buf, bufSize, fmt, args);
	if (w == -1 || w >= bufSize) w = bufSize - 1;
	buf[w] = 0;

	logWindow.AddLog(id, buf);
}

void AddLog(int id, const std::string& text)
{
	logWindow.AddLog(id, text);
}

void ClearLog(int id)
{
	logWindow.ClearLog(id);
}

#define LOG_COMMON(id)          \
	do                          \
	{                           \
		va_list args;           \
		va_start(args, fmt);    \
		AddLogV(id, fmt, args); \
		va_end(args);           \
	}                           \
	while (0)

void RemovePrevious(std::string_view prevText)
{
	logWindow.RemovePrevious(prevText);
}

void Tab(int value)
{
	logWindow.Tab(value);
}

} // namespace ui
