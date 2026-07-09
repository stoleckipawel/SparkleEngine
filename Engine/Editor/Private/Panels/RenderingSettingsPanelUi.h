#pragma once

#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <cfloat>
#include <cstdint>
#include <string>

namespace RenderingSettingsPanelUi
{
	inline constexpr float kLabelColumnWidth = 340.0f;

	template <typename ValueType>
	struct ComboOption final
	{
		const char* Label = "";
		ValueType Value{};
	};

	inline bool MatchesFilter(const char* filterText, const char* title, const char* keywords)
	{
		if (filterText == nullptr || filterText[0] == '\0')
		{
			return true;
		}

		return UiUtil::MatchesDetailsFilter(std::string(filterText), title, keywords);
	}

	inline bool BeginSettingsCategory(const char* label)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, SparkleUiPalette::HeaderBackground());
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SparkleUiPalette::HeaderBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, SparkleUiPalette::HeaderBackgroundActive());
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
		const bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);
		return open;
	}

	inline bool BeginSettingsTable(const char* id)
	{
		const ImGuiTableFlags tableFlags =
		    ImGuiTableFlags_SizingStretchProp |
		    ImGuiTableFlags_BordersInnerV |
		    ImGuiTableFlags_BordersInnerH;
		if (!ImGui::BeginTable(id, 2, tableFlags))
		{
			return false;
		}

		ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	}

	template <typename OnChanged>
	void DrawBooleanRow(const char* id, const char* label, bool value, OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		bool updatedValue = value;
		if (ImGui::Checkbox(id, &updatedValue))
		{
			onChanged(updatedValue);
		}
	}

	template <typename ValueType, std::size_t OptionCount, typename OnChanged>
	void DrawComboOptionRow(
	    const char* id,
	    const char* label,
	    ValueType value,
	    const ComboOption<ValueType> (&options)[OptionCount],
	    OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		const char* previewLabel = OptionCount > 0 ? options[0].Label : "";
		for (const ComboOption<ValueType>& option : options)
		{
			if (option.Value == value)
			{
				previewLabel = option.Label;
				break;
			}
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo(id, previewLabel))
		{
			for (const ComboOption<ValueType>& option : options)
			{
				const bool selected = option.Value == value;
				if (ImGui::Selectable(option.Label, selected))
				{
					onChanged(option.Value);
				}
				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	template <typename OnChanged>
	void DrawUnsignedIntInputRow(
	    const char* id,
	    const char* label,
	    std::uint32_t value,
	    OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		int updatedValue = static_cast<int>(value);
		if (ImGui::InputInt(id, &updatedValue, 1, 16))
		{
			onChanged(static_cast<std::uint32_t>((std::max)(updatedValue, 0)));
		}
	}

	template <typename OnChanged>
	void DrawUnsignedIntSliderRow(
	    const char* id,
	    const char* label,
	    std::uint32_t value,
	    std::uint32_t minValue,
	    std::uint32_t maxValue,
	    OnChanged&& onChanged)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		int updatedValue = static_cast<int>(value);
		const int minInt = static_cast<int>(minValue);
		const int maxInt = static_cast<int>(maxValue);
		if (ImGui::SliderInt(id, &updatedValue, minInt, maxInt))
		{
			onChanged(static_cast<std::uint32_t>(updatedValue));
		}
	}

	template <typename OnChanged>
	void DrawFloatInputRow(
	    const char* id,
	    const char* label,
	    float value,
	    OnChanged&& onChanged,
	    float step = 1.0f,
	    float stepFast = 10.0f,
	    const char* format = "%.3f")
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		float updatedValue = value;
		if (ImGui::InputFloat(id, &updatedValue, step, stepFast, format))
		{
			onChanged(updatedValue);
		}
	}
}
