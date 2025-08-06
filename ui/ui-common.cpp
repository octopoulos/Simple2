// ui-common.cpp
// @author octopoulos
// @version 2025-08-01

#include "stdafx.h"
#include "ui/ui.h"
//
#include "ui/xsettings.h"

namespace ui
{

static std::unordered_map<std::string, uint32_t> textures;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

bool AddCombo(const std::string& name, const char* text)
{
	bool result = false;
	if (auto config = ConfigFind(name))
	{
		result = ImGui::Combo(text, (int*)config->ptr, config->names, config->count);
		result |= ItemEvent(name);
	}
	return result;
}

bool AddCombo(const std::string& name, const char* text, const char* texts[], const VEC_INT values)
{
	bool result = false;
	if (auto config = ConfigFind(name))
	{
		auto it    = std::find(values.begin(), values.end(), *(int*)config->ptr);
		int  index = (it != values.end()) ? TO_INT(std::distance(values.begin(), it)) : 0;

		if (ImGui::Combo(text, &index, texts, TO_INT(values.size())))
		{
			*(int*)config->ptr = values[index];
			result             = true;
		}
		result |= ItemEvent(name);
	}
	return result;
}

/// Same as DraggScalarN but can be reset with right click
/// @param mode: 0:slider, 1:drag
static bool AddDragScalarN(int mode, const std::string& name, const char* label, ImGuiDataType data_type, size_t type_size, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags = 0)
{
	const float spacingX      = ImGui::GetStyle().ItemInnerSpacing.x;
	bool        value_changed = false;

	ImGui::BeginGroup();
	ImGui::PushID(label);
	components = std::max(components, 1);
	ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
	for (int i = 0; i < components; ++i)
	{
		ImGui::PushID(i);
		if (i > 0)
			ImGui::SameLine(0, spacingX);

		if (mode == 0)
			value_changed |= ImGui::SliderScalar(fmt::format("##{}{}", name, i).c_str(), data_type, p_data, p_min, p_max, format, flags);
		else
			value_changed |= ImGui::DragScalar(fmt::format("##{}{}", name, i).c_str(), data_type, p_data, v_speed, p_min, p_max, format, flags);

		value_changed |= ItemEvent(name, i);

		ImGui::SameLine();
		ImGui::PopID();
		ImGui::PopItemWidth();
		p_data = (void*)((char*)p_data + type_size);
	}
	ImGui::PopID();

	if (auto label_end = ImGui::FindRenderedTextEnd(label); label != label_end)
	{
		ImGui::SameLine(0, spacingX);
		ImGui::TextEx(label, label_end);
	}

	ImGui::EndGroup();
	return value_changed;
}

bool AddDragFloat(const std::string& name, const char* text, float speed, const char* format)
{
	bool result = false;
	if (auto config = ConfigFind(name))
		result = AddDragScalarN(1, name, text, ImGuiDataType_Float, sizeof(float), (float*)config->ptr, config->count, speed, &config->minFloat, &config->maxFloat, format);
	return result;
}

bool AddDragInt(const std::string& name, const char* text, float speed, const char* format)
{
	bool result = false;
	if (auto config = ConfigFind(name))
		result = AddDragScalarN(1, name, text, ImGuiDataType_S32, sizeof(int32_t), (int*)config->ptr, config->count, speed, &config->minInt, &config->maxFloat, format);
	return result;
}

bool AddSliderBool(const std::string& name, const char* text, const char* format, bool vertical, const ImVec2& size)
{
	return AddSliderInt(name, text, format, vertical, size, true);
}

bool AddSliderFloat(const std::string& name, const char* text, const char* format)
{
	bool result = false;
	if (auto config = ConfigFind(name))
		result = AddDragScalarN(0, name, text, ImGuiDataType_Float, sizeof(float), (float*)config->ptr, config->count, 1.0f, &config->minFloat, &config->maxFloat, format);
	return result;
}

bool AddSliderInt(const std::string& name, const char* text, const char* format, bool vertical, const ImVec2& size, bool isBool)
{
	bool result = false;
	if (auto config = ConfigFind(name))
	{
		// slider enum?
		const int count   = config->type == 'e' ? 1 : config->count;
		const int value   = *(int*)config->ptr;
		auto      format2 = (isBool ? ((value == 0) ? "Off" : "On") : (format ? format : ((value >= 0 && value < config->count) ? config->names[value] : "%d")));
		const int min     = format ? config->minInt : 0;
		const int max     = format ? config->maxInt : config->count - 1;

		if (count == 1 && vertical)
		{
			result = ImGui::VSliderInt(text, size, (int*)config->ptr, min, max, format2);
			result |= ItemEvent(name);
		}
		else result = AddDragScalarN(0, name, text, ImGuiDataType_S32, sizeof(int32_t), (int*)config->ptr, count, 1.0f, &min, &max, format2);
	}
	return result;
}

bool AddSliderInt(const std::string& name, const char* text, int* value, int count, int min, int max, const char* format)
{
	return AddDragScalarN(0, name, text, ImGuiDataType_S32, sizeof(int32_t), value, count, 1.0f, &min, &max, format);
}

void AddSpace(float height)
{
	if (height < 0) height = -height * ImGui::GetStyle().WindowPadding.y;
	ImGui::Dummy(ImVec2(0, height * xsettings.uiScale));
}

bool ItemEvent(const std::string& name, int index)
{
	if (auto config = ConfigFind(name))
	{
		if (ImGui::IsItemClicked(1))
		{
			config->ResetDefault(index);
			return true;
		}
	}
	return false;
}

void ShowTable(const std::vector<std::tuple<std::string, std::string>>& stats)
{
	if (ImGui::BeginTable("Stats", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
	{
		for (const auto& [name, value] : stats)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(value.c_str());
		}
		ImGui::EndTable();
	}
}


static CommonWindow commonWindow;

CommonWindow& GetCommonWindow() { return commonWindow; }

} // namespace ui
