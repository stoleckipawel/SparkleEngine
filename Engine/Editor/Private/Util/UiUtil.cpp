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
		constexpr float DetailsLabelWidth = 148.0f;
		constexpr float DetailsAxisLabelWidth = 14.0f;
		constexpr float DetailsRowVerticalPadding = 3.0f;
		constexpr float PlaceholderIconSize = 16.0f;
		constexpr float DetailsUtilityColumnWidth = 24.0f;

		ImVec4 DetailsGridLineColor() noexcept
		{
			ImVec4 color = ImGui::ColorConvertU32ToFloat4(SparkleUiPalette::PanelHeaderBorder());
			color.w = 0.28f;
			return color;
		}

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

		ImU32 AxisColor(int axisIndex) noexcept
		{
			switch (axisIndex)
			{
				case 0:
					return IM_COL32(216, 92, 92, 210);
				case 1:
					return IM_COL32(92, 190, 112, 210);
				case 2:
					return IM_COL32(92, 132, 220, 210);
				default:
					return ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::TextMuted());
			}
		}

		bool BeginDetailsRow(const char* label, int valueColumnCount)
		{
			ImGui::PushID(label);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, DetailsRowVerticalPadding));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
			const ImVec4 gridLineColor = DetailsGridLineColor();
			ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, gridLineColor);
			ImGui::PushStyleColor(ImGuiCol_TableBorderLight, gridLineColor);
			ImGui::PushStyleColor(ImGuiCol_Separator, gridLineColor);

			ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchSame;
			tableFlags |= ImGuiTableFlags_NoPadOuterX;
			tableFlags |= ImGuiTableFlags_BordersInnerV;

			if (!ImGui::BeginTable("##details_row", valueColumnCount + 2, tableFlags))
			{
				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar(2);
				ImGui::PopID();
				return false;
			}

			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, DetailsLabelWidth);
			for (int valueColumnIndex = 0; valueColumnIndex < valueColumnCount; ++valueColumnIndex)
			{
				ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
			}
			ImGui::TableSetupColumn("utility", ImGuiTableColumnFlags_WidthFixed, DetailsUtilityColumnWidth);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::TextDisabled("%s", label);
			return true;
		}

		bool DrawDetailsResetButton(int utilityColumnIndex, const char* tooltip = "Reset to default")
		{
			ImGui::TableSetColumnIndex(utilityColumnIndex);
			return DrawEditorIconButton(EditorIcon::Reset, "reset", tooltip);
		}

		void DrawDetailsEmptyUtility(int utilityColumnIndex)
		{
			ImGui::TableSetColumnIndex(utilityColumnIndex);
			ImGui::Dummy(ImVec2(DetailsUtilityColumnWidth, PlaceholderIconSize));
		}

		void EndDetailsRow()
		{
			ImGui::EndTable();
			ImGui::Separator();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);
			ImGui::PopID();
		}
	}  // namespace

	const char* GetEditorIconGlyph(EditorIcon icon) noexcept
	{
		switch (icon)
		{
			case EditorIcon::Camera:
				return "C";
			case EditorIcon::DirectionalLight:
				return "L";
			case EditorIcon::StaticMesh:
				return "M";
			case EditorIcon::EyeVisible:
				return "O";
			case EditorIcon::EyeHidden:
				return "-";
			case EditorIcon::Reset:
				return "R";
			case EditorIcon::Filter:
				return "F";
			case EditorIcon::Settings:
				return "S";
			case EditorIcon::None:
			default:
				return "-";
		}
	}

	void DrawEditorIcon(EditorIcon icon, const char* tooltip)
	{
		DrawPlaceholderTypeIcon(GetEditorIconGlyph(icon), tooltip);
	}

	bool DrawEditorIconButton(EditorIcon icon, const char* id, const char* tooltip)
	{
		ImGui::PushID(id);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SparkleUiPalette::ButtonBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, SparkleUiPalette::ButtonBackgroundActive());
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
		const bool pressed = ImGui::Button(GetEditorIconGlyph(icon), ImVec2(PlaceholderIconSize, PlaceholderIconSize));
		if (tooltip != nullptr && tooltip[0] != '\0' && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return pressed;
	}

	void DrawPlaceholderTypeIcon(const char* text, const char* tooltip)
	{
		const ImVec2 size(PlaceholderIconSize, PlaceholderIconSize);
		const ImVec2 start = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##placeholder_type_icon", size);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 end(start.x + size.x, start.y + size.y);
		drawList->AddRectFilled(start, end, SparkleUiPalette::SceneOutlinerBadgeBackground(), 3.0f);
		drawList->AddRect(start, end, SparkleUiPalette::PanelHeaderBorder(), 3.0f, 0, 1.0f);

		if (text != nullptr && text[0] != '\0')
		{
			const ImVec2 textSize = ImGui::CalcTextSize(text);
			const ImVec2 textPos(start.x + ((size.x - textSize.x) * 0.5f), start.y + ((size.y - textSize.y) * 0.5f));
			drawList->AddText(textPos, SparkleUiPalette::SceneOutlinerBadgeText(), text);
		}

		if (tooltip != nullptr && tooltip[0] != '\0' && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
	}

	bool DrawVisibilityIconButton(const char* id, bool visible)
	{
		ImGui::PushID(id);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SparkleUiPalette::ButtonBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, SparkleUiPalette::ButtonBackgroundActive());
		ImGui::PushStyleColor(ImGuiCol_Text, visible ? SparkleUiPalette::TextPrimary() : SparkleUiPalette::TextMuted());
		const bool pressed = ImGui::Button(
		    GetEditorIconGlyph(visible ? EditorIcon::EyeVisible : EditorIcon::EyeHidden),
		    ImVec2(PlaceholderIconSize, PlaceholderIconSize));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", visible ? "Visible" : "Hidden");
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return pressed;
	}

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
		ImGui::TextUnformatted(value);
		DrawDetailsEmptyUtility(2);

		EndDetailsRow();
	}

	bool EditDetailsFloat(
	    const char* label,
	    float& value,
	    float speed,
	    float minValue,
	    float maxValue,
	    const char* format,
	    const float* resetValue)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		bool changed = ImGui::DragFloat("##value", &value, speed, minValue, maxValue, format);
		if (resetValue != nullptr)
		{
			if (DrawDetailsResetButton(2))
			{
				value = *resetValue;
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsFloat3(
	    const char* label,
	    float values[3],
	    float speed,
	    float minValue,
	    float maxValue,
	    const char* format,
	    const float* resetValues)
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
				ImGui::PushStyleColor(ImGuiCol_Text, AxisColor(axisIndex));
				ImGui::TextUnformatted(axisLabels[axisIndex]);
				ImGui::PopStyleColor();

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed = ImGui::DragFloat("##value", &values[axisIndex], speed, minValue, maxValue, format) || changed;

				ImGui::EndTable();
			}
			ImGui::PopID();
		}
		if (resetValues != nullptr)
		{
			if (DrawDetailsResetButton(4))
			{
				values[0] = resetValues[0];
				values[1] = resetValues[1];
				values[2] = resetValues[2];
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(4);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsColor3(const char* label, float values[3], const float* resetValues)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		bool changed = ImGui::ColorEdit3(
		    "##value",
		    values,
		    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar);
		if (resetValues != nullptr)
		{
			if (DrawDetailsResetButton(2))
			{
				values[0] = resetValues[0];
				values[1] = resetValues[1];
				values[2] = resetValues[2];
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsCheckbox(const char* label, bool& value, const bool* resetValue)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		bool changed = ImGui::Checkbox("##value", &value);
		if (resetValue != nullptr)
		{
			if (DrawDetailsResetButton(2))
			{
				value = *resetValue;
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

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