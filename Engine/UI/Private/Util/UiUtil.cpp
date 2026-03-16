#include "PCH.h"
#include "Util/UiUtil.h"

#include "Style/SparkleUiTheme.h"

#include <algorithm>

#include <imgui.h>

namespace UiUtil
{
	namespace
	{
		constexpr float PropertyLabelWidth = 78.0f;
		constexpr float ScalarInputWidth = 86.0f;
		constexpr float Float3InputWidth = 146.0f;

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

		void DrawHeaderBar(const char* title, float height, ImU32 backgroundColor, ImU32 borderColor, ImFont* font, const ImVec2& padding)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 start = ImGui::GetCursorScreenPos();
			const float width = ImGui::GetContentRegionAvail().x;
			const ImVec2 end = ImVec2(start.x + width, start.y + height);

			drawList->AddRectFilled(start, end, backgroundColor);
			drawList->AddLine(ImVec2(start.x, end.y - 1.0f), ImVec2(end.x, end.y - 1.0f), borderColor, 1.0f);

			ImGui::InvisibleButton("##header_bar", ImVec2(width, height));
			ImGui::SetCursorScreenPos(ImVec2(start.x + padding.x, start.y + padding.y));
			PushFontIfAvailable(font);
			ImGui::TextUnformatted(title);
			PopFontIfAvailable(font);
			ImGui::SetCursorScreenPos(ImVec2(start.x, end.y + 4.0f));
		}

		void DrawRightAlignedText(const char* value)
		{
			const float valueWidth = ImGui::CalcTextSize(value).x;
			const float currentX = ImGui::GetCursorPosX();
			const float offset = (std::max)(0.0f, ImGui::GetContentRegionAvail().x - valueWidth);
			ImGui::SetCursorPosX(currentX + offset);
			ImGui::TextUnformatted(value);
		}
	}

	void DrawPanelHeader(const char* title, const char* subtitle)
	{
		ImFont* headingFont = SparkleUiTheme::GetHeadingFont();
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		DrawHeaderBar(title, 28.0f, IM_COL32(34, 34, 37, 255), IM_COL32(72, 72, 78, 255), headingFont, ImVec2(8.0f, 4.0f));

		if (subtitle != nullptr && subtitle[0] != '\0')
		{
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", subtitle);
			PopFontIfAvailable(monoFont);
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
		}
	}

	void BeginSectionCard(const char* title)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
		ImGui::BeginChild(title, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawSectionHeader(title);
	}

	void EndSectionCard()
	{
		ImGui::EndChild();
		ImGui::PopStyleVar(3);
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
			if (ImGui::BeginTable("##float_editor", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthFixed, ScalarInputWidth);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1.0f);
				changedBySlider = ImGui::SliderFloat("##slider", &value, minValue, maxValue, sliderFormat);

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
		bool changedBySlider = false;
		bool changedByInput = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable("##float3_row", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
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
			if (ImGui::BeginTable("##float3_editor", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthFixed, Float3InputWidth);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1.0f);
				changedBySlider = ImGui::SliderFloat3("##slider", values, minValue, maxValue, sliderFormat);

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1.0f);
				changedByInput = ImGui::InputFloat3("##input", values, inputFormat);

				ImGui::EndTable();
			}

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changedBySlider || changedByInput;
	}

	void DrawSectionHeader(const char* title)
	{
		ImFont* headingFont = SparkleUiTheme::GetHeadingFont();
		ImGui::PushID(title);
		DrawHeaderBar(title, 22.0f, IM_COL32(44, 46, 50, 255), IM_COL32(70, 74, 80, 255), headingFont, ImVec2(6.0f, 3.0f));
		ImGui::PopID();
	}
}