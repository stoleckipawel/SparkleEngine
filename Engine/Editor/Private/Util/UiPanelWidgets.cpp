#include "PCH.h"
#include "Util/UiUtil.h"

#include "Style/SparkleUiPalette.h"
#include "Style/SparkleUiTheme.h"
#include "Util/EditorIconGlyphs.h"

#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

#include <imgui.h>

#include "Util/UiUtilInternal.h"

namespace UiUtil
{
	using namespace Internal;
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
}
