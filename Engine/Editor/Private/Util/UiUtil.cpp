#include "PCH.h"
#include "Util/UiUtil.h"

#include "Style/SparkleUiPalette.h"
#include "Style/SparkleUiTheme.h"

#include <algorithm>
#include <cfloat>

#include <imgui.h>

namespace UiUtil
{
	namespace
	{
		constexpr float PropertyLabelWidth = 78.0f;
		constexpr float ScalarInputWidth = 86.0f;
		constexpr float Float3InputWidth = 188.0f;
		constexpr float DetailsLabelWidth = 118.0f;
		constexpr float DetailsAxisLabelWidth = 14.0f;
		constexpr float DetailsRowVerticalPadding = 2.0f;

		void PushFontIfAvailable(ImFont* font)
		{
			if (font != nullptr)
			{
				ImGui::PushFont(font);
			}
		}

		void PopFontIfAvailable(ImFont* font)
		{
			if (font != nullptr)
			{
				ImGui::PopFont();
			}
		}

		void DrawHeaderBar(
		    const char* title,
		    const char* trailingText,
		    float height,
		    ImU32 backgroundColor,
		    ImU32 borderColor,
		    ImFont* titleFont,
		    ImFont* trailingFont,
		    const ImVec2& padding)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float width = ImGui::GetContentRegionAvail().x;
			const ImVec2 end = ImVec2(start.x + width, start.y + height);

			drawList->AddRectFilled(start, end, backgroundColor);
			drawList->AddLine(ImVec2(start.x, end.y - 1.0f), ImVec2(end.x, end.y - 1.0f), borderColor, 1.0f);

			ImGui::InvisibleButton("##header_bar", ImVec2(width, height));
			ImGui::SetCursorScreenPos(ImVec2(start.x + padding.x, start.y + padding.y));
			PushFontIfAvailable(titleFont);
			ImGui::TextUnformatted(title);
			PopFontIfAvailable(titleFont);

			if (trailingText != nullptr && trailingText[0] != '\0')
			{
				const float trailingWidth = ImGui::CalcTextSize(trailingText).x;
				const float trailingRightPadding = padding.x + ImGui::GetStyle().ScrollbarSize;
				ImGui::SetCursorScreenPos(ImVec2(end.x - trailingRightPadding - trailingWidth, start.y + padding.y));
				PushFontIfAvailable(trailingFont);
				ImGui::TextDisabled("%s", trailingText);
				PopFontIfAvailable(trailingFont);
			}

			ImGui::SetCursorScreenPos(ImVec2(start.x, end.y + 4.0f));
		}

		void DrawRightAlignedText(const char* value)
		{
			const float valueWidth = ImGui::CalcTextSize(value).x;
			const float currentX = ImGui::GetCursorPosX();
			const float offset = (std::max) (0.0f, ImGui::GetContentRegionAvail().x - valueWidth);
			ImGui::SetCursorPosX(currentX + offset);
			ImGui::TextUnformatted(value);
		}

		bool BeginDetailsRow(const char* label, int valueColumnCount)
		{
			ImGui::PushID(label);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, DetailsRowVerticalPadding));
			if (!ImGui::BeginTable("##details_row", valueColumnCount + 1, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX))
			{
				ImGui::PopStyleVar();
				ImGui::PopID();
				return false;
			}

			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, DetailsLabelWidth);
			for (int valueColumnIndex = 0; valueColumnIndex < valueColumnCount; ++valueColumnIndex)
			{
				ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled("%s", label);
			return true;
		}

		void EndDetailsRow()
		{
			ImGui::EndTable();
			ImGui::PopStyleVar();
			ImGui::PopID();
		}
	}  // namespace

	void DrawPanelHeader(const char* title, const char* subtitle)
	{
		ImFont* headingFont = SparkleUiTheme::GetHeadingFont();
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		DrawHeaderBar(
		    title,
		    subtitle,
		    24.0f,
		    SparkleUiPalette::PanelHeaderBackground(),
		    SparkleUiPalette::PanelHeaderBorder(),
		    headingFont,
		    monoFont,
		    ImVec2(8.0f, 3.0f));
	}

	void BeginSectionCard(const char* title)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginChild(
		    title,
		    ImVec2(0.0f, 0.0f),
		    ImGuiChildFlags_AutoResizeY,
		    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawSectionHeader(title);
	}

	void EndSectionCard()
	{
		ImGui::Spacing();
		ImGui::EndChild();
		ImGui::PopStyleVar(1);
	}

	void DrawKeyValueRow(const char* label, const char* value)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		ImGui::PushID(label);
		if (ImGui::BeginTable("##kv_row", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			PushFontIfAvailable(monoFont);
			DrawRightAlignedText(value);
			PopFontIfAvailable(monoFont);

			ImGui::EndTable();
		}
		ImGui::PopID();
	}

	bool EditFloatSliderWithInput(
	    const char* label,
	    float& value,
	    float minValue,
	    float maxValue,
	    const char* sliderFormat,
	    const char* inputFormat)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		const char* sliderValueFormat = (inputFormat != nullptr && inputFormat[0] != '\0') ? "" : sliderFormat;
		bool changedBySlider = false;
		bool changedByInput = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable("##float_row", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			if (ImGui::BeginTable(
			        "##float_editor",
			        2,
			        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthFixed, ScalarInputWidth);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1.0f);
				changedBySlider = ImGui::SliderFloat("##slider", &value, minValue, maxValue, sliderValueFormat);

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1.0f);
				changedByInput = ImGui::InputFloat("##input", &value, 0.0f, 0.0f, inputFormat);

				ImGui::EndTable();
			}

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changedBySlider || changedByInput;
	}

	bool EditFloat3SliderWithInput(
	    const char* label,
	    float values[3],
	    float minValue,
	    float maxValue,
	    const char* sliderFormat,
	    const char* inputFormat)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		const char* sliderValueFormat = (inputFormat != nullptr && inputFormat[0] != '\0') ? "" : sliderFormat;
		bool changedBySlider = false;
		bool changedByInput = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable(
		        "##float3_row",
		        2,
		        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			if (ImGui::BeginTable(
			        "##float3_editor",
			        1,
			        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1.0f);
				changedBySlider = ImGui::SliderFloat3("##slider", values, minValue, maxValue, sliderValueFormat);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(Float3InputWidth);
				changedByInput = ImGui::InputFloat3("##input", values, inputFormat) || changedByInput;

				ImGui::EndTable();
			}

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changedBySlider || changedByInput;
	}

	bool EditColor3(const char* label, float values[3])
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		bool changed = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable(
		        "##color3_row",
		        2,
		        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-1.0f);
			changed = ImGui::ColorEdit3(
			    "##color",
			    values,
			    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB |
			        ImGuiColorEditFlags_PickerHueBar);

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changed;
	}

	bool EditCheckbox(const char* label, bool& value)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		bool changed = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable(
		        "##checkbox_row",
		        2,
		        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			changed = ImGui::Checkbox("##checkbox", &value);

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changed;
	}

	bool BeginDetailsCategory(const char* title, bool defaultOpen)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed;
		if (defaultOpen)
		{
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));
		ImGui::PushStyleColor(ImGuiCol_Header, SparkleUiPalette::SectionHeaderBackground());
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SparkleUiPalette::HeaderBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, SparkleUiPalette::HeaderBackgroundActive());
		const bool open = ImGui::CollapsingHeader(title, flags);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		return open;
	}

	void EndDetailsCategory()
	{
		ImGui::Spacing();
	}

	void DrawDetailsValueRow(const char* label, const char* value)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		DrawRightAlignedText(value);

		EndDetailsRow();
	}

	bool EditDetailsFloat(const char* label, float& value, float speed, float minValue, float maxValue, const char* format)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		const bool changed = ImGui::DragFloat("##value", &value, speed, minValue, maxValue, format);

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsFloat3(const char* label, float values[3], float speed, float minValue, float maxValue, const char* format)
	{
		if (!BeginDetailsRow(label, 3))
		{
			return false;
		}

		bool changed = false;
		static constexpr const char* axisLabels[] = {"X", "Y", "Z"};
		for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
		{
			ImGui::PushID(axisIndex);
			ImGui::TableSetColumnIndex(axisIndex + 1);
			if (ImGui::BeginTable(
			        "##axis_field",
			        2,
			        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("axis", ImGuiTableColumnFlags_WidthFixed, DetailsAxisLabelWidth);
				ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled("%s", axisLabels[axisIndex]);

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed = ImGui::DragFloat("##value", &values[axisIndex], speed, minValue, maxValue, format) || changed;

				ImGui::EndTable();
			}
			ImGui::PopID();
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsCheckbox(const char* label, bool& value)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		const bool changed = ImGui::Checkbox("##value", &value);

		EndDetailsRow();
		return changed;
	}

	void DrawSectionHeader(const char* title)
	{
		ImFont* headingFont = SparkleUiTheme::GetHeadingFont();
		ImGui::PushID(title);
		DrawHeaderBar(
		    title,
		    nullptr,
		    22.0f,
		    SparkleUiPalette::SectionHeaderBackground(),
		    SparkleUiPalette::SectionHeaderBorder(),
		    headingFont,
		    nullptr,
		    ImVec2(6.0f, 3.0f));
		ImGui::PopID();
	}
}  // namespace UiUtil