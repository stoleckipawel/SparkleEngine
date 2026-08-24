#include "PCH.h"

#include "Panels/UsedTexturesPanel.h"

#include "Panels/Diagnostics/PanelDiagnosticsFormatting.h"
#include "Panels/Diagnostics/TextureDiagnosticsPresentation.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>
#include <utility>

#include <imgui.h>

void UsedTexturesPanel::SetDiagnosticsProvider(DiagnosticsProvider provider)
{
	m_diagnosticsProvider = std::move(provider);
}

void UsedTexturesPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen)
	{
		return;
	}

	RefreshSnapshot();
	ImGui::SetNextWindowSize(ImVec2(1280.0f, 760.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Material, "Texture Tools") + "##Texture Tools";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawToolbar();
	ImGui::Separator();

	const ImGuiTableFlags layoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
	if (ImGui::BeginTable("##TextureInspectorLayout", 2, layoutFlags, ImGui::GetContentRegionAvail()))
	{
		ImGui::TableSetupColumn("Textures", ImGuiTableColumnFlags_WidthStretch, 0.62f);
		ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.38f);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::BeginChild("##TextureTablePane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
		DrawTextureTable(disableInteraction);
		ImGui::EndChild();
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginChild(
		    "##TexturePreviewPane",
		    ImVec2(0.0f, 0.0f),
		    ImGuiChildFlags_None,
		    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawSelectedTextureInspector(disableInteraction);
		ImGui::EndChild();
		ImGui::EndTable();
	}

	ImGui::End();
}

void UsedTexturesPanel::RefreshSnapshot()
{
	if (m_diagnosticsProvider)
	{
		m_snapshot = m_diagnosticsProvider();
	}
}

void UsedTexturesPanel::DrawToolbar()
{
	ImGui::SetNextItemWidth(300.0f);
	const std::string filterHint = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Search, "Filter texture/path/format");
	ImGui::InputTextWithHint("##UsedTexturesFilter", filterHint.c_str(), m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::SameLine();
	ImGui::TextDisabled(
	    "%zu texture(s), %s estimated",
	    m_snapshot.size(),
	    PanelDiagnosticsFormatting::FormatByteSize(TextureDiagnosticsPresentation::SumEstimatedTextureBytes(m_snapshot)).c_str());
}

void UsedTexturesPanel::DrawTextureTable(bool disableInteraction)
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable
	    | ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
	if (!ImGui::BeginTable("##UsedTexturesTable", 9, tableFlags, ImVec2(0.0f, 0.0f)))
	{
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Texture");
	ImGui::TableSetupColumn("Source");
	ImGui::TableSetupColumn("Kind");
	ImGui::TableSetupColumn("Loaded");
	ImGui::TableSetupColumn("Resident");
	ImGui::TableSetupColumn("Format");
	ImGui::TableSetupColumn("Size");
	ImGui::TableSetupColumn("Mips");
	ImGui::TableSetupColumn("Memory");
	ImGui::TableHeadersRow();

	ImGui::BeginDisabled(disableInteraction);
	for (const TextureDiagnosticsRow& row : m_snapshot)
	{
		if (!MatchesFilter(row))
		{
			continue;
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		const bool selected = row.Key == m_selectedKey;
		const std::string displayName = TextureDiagnosticsPresentation::FormatTextureDisplayName(row);
		if (ImGui::Selectable(displayName.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
		{
			m_selectedKey = row.Key;
		}
		ImGui::TableNextColumn();
		const std::string sourcePath = TextureDiagnosticsPresentation::FormatTexturePath(row);
		ImGui::TextUnformatted(sourcePath.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(TextureDiagnosticsPresentation::FormatTextureKind(row.Kind));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.Loaded ? "yes" : "no");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(TextureDiagnosticsPresentation::FormatResidency(row.ResidencyState));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(TextureDiagnosticsPresentation::FormatTextureFormat(row.Format));
		ImGui::TableNextColumn();
		const std::string extent = TextureDiagnosticsPresentation::FormatExtent(row);
		ImGui::TextUnformatted(extent.c_str());
		ImGui::TableNextColumn();
		ImGui::Text("%u", static_cast<unsigned int>(row.MipCount));
		ImGui::TableNextColumn();
		const std::string memory = PanelDiagnosticsFormatting::FormatByteSize(row.EstimatedByteSize);
		ImGui::TextUnformatted(memory.c_str());
	}
	ImGui::EndDisabled();
	ImGui::EndTable();
}

void UsedTexturesPanel::DrawSelectedTextureInspector(bool disableInteraction)
{
	const TextureDiagnosticsRow* selectedRow = GetSelectedRow();
	if (selectedRow == nullptr)
	{
		ImGui::TextDisabled("Select a texture to inspect preview, dimensions, format, memory, and runtime residency.");
		return;
	}

	const std::string displayName = TextureDiagnosticsPresentation::FormatTextureDisplayName(*selectedRow);
	const std::string sourcePath = TextureDiagnosticsPresentation::FormatTexturePath(*selectedRow);
	ImGui::TextUnformatted(displayName.c_str());
	PanelDiagnosticsFormatting::DrawWrappedDisabledText(sourcePath);
	ImGui::Separator();
	DrawPreview(*selectedRow);
	ImGui::SeparatorText("Runtime Properties");
	ImGui::BeginChild("##TextureDetailsPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
	DrawSelectedTextureDetails(*selectedRow);
	ImGui::EndChild();
}

void UsedTexturesPanel::DrawPreview(const TextureDiagnosticsRow& row) const
{
	const float detailsReserveHeight = 168.0f;
	ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	availableRegion.y = (std::max) (96.0f, availableRegion.y - detailsReserveHeight);

	if (!row.PreviewTexture || row.Dimension != TextureResourceDimension::Texture2D)
	{
		ImGui::BeginChild("##TexturePreviewUnavailable", ImVec2(0.0f, availableRegion.y), ImGuiChildFlags_Borders);
		ImGui::TextDisabled(
		    row.Dimension == TextureResourceDimension::TextureCube ? "Cubemap preview is not available in v1."
		                                                           : "No preview texture is available.");
		ImGui::EndChild();
		return;
	}

	ImGui::BeginChild("##TexturePreview", ImVec2(0.0f, availableRegion.y), ImGuiChildFlags_Borders);
	const ImVec2 imageSize = TextureDiagnosticsPresentation::ComputePreviewImageSize(ImGui::GetContentRegionAvail(), row.Width, row.Height);
	const ImVec2 start = ImGui::GetCursorPos();
	const ImVec2 childRegion = ImGui::GetContentRegionAvail();
	if (childRegion.x > imageSize.x)
	{
		ImGui::SetCursorPosX(start.x + ((childRegion.x - imageSize.x) * 0.5f));
	}
	if (childRegion.y > imageSize.y)
	{
		ImGui::SetCursorPosY(start.y + ((childRegion.y - imageSize.y) * 0.5f));
	}
	ImGui::Image(static_cast<ImTextureID>(row.PreviewTexture.Pack()), imageSize);
	ImGui::EndChild();
}

void UsedTexturesPanel::DrawSelectedTextureDetails(const TextureDiagnosticsRow& row) const
{
	const std::string extent = TextureDiagnosticsPresentation::FormatExtent(row);
	const std::string memory = PanelDiagnosticsFormatting::FormatByteSize(row.EstimatedByteSize);
	const std::string mips = std::to_string(row.MipCount);
	const std::string arraySize = std::to_string(row.ArraySize);
	const char* textureFormat = TextureDiagnosticsPresentation::FormatTextureFormat(row.Format);

	if (const std::optional<std::string> sourcePath = TextureDiagnosticsPresentation::FindAuthoredSourcePath(row))
	{
		UiUtil::DrawKeyValueRow("Source", sourcePath->c_str());
	}
	UiUtil::DrawKeyValueRow("Kind", TextureDiagnosticsPresentation::FormatTextureKind(row.Kind));
	UiUtil::DrawKeyValueRow("Loaded", row.Loaded ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Resident", TextureDiagnosticsPresentation::FormatResidency(row.ResidencyState));
	UiUtil::DrawKeyValueRow("Streamed", row.StreamManaged ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Default", row.Kind == TextureDiagnosticsKind::Scene ? "no" : "yes");
	UiUtil::DrawKeyValueRow("Dimension", TextureDiagnosticsPresentation::FormatTextureDimension(row.Dimension));
	UiUtil::DrawKeyValueRow("Size", extent.c_str());
	UiUtil::DrawKeyValueRow("Array", arraySize.c_str());
	UiUtil::DrawKeyValueRow("Mips", mips.c_str());
	UiUtil::DrawKeyValueRow("Format", textureFormat);
	UiUtil::DrawKeyValueRow("Intent", TextureDiagnosticsPresentation::FormatTextureIntent(row.FormatIntent));
	UiUtil::DrawKeyValueRow("Memory", memory.c_str());
	UiUtil::DrawKeyValueRow("Key", row.Key.c_str());
}

const TextureDiagnosticsRow* UsedTexturesPanel::GetSelectedRow() const noexcept
{
	for (const TextureDiagnosticsRow& row : m_snapshot)
	{
		if (row.Key == m_selectedKey)
		{
			return &row;
		}
	}
	return nullptr;
}

bool UsedTexturesPanel::MatchesFilter(const TextureDiagnosticsRow& row) const noexcept
{
	const std::string_view filter(m_filterBuffer.data());
	return filter.empty() || Strings::ContainsIgnoreCase(TextureDiagnosticsPresentation::FormatTextureDisplayName(row), filter)
	    || Strings::ContainsIgnoreCase(TextureDiagnosticsPresentation::FormatTexturePath(row), filter)
	    || Strings::ContainsIgnoreCase(row.Key, filter)
	    || Strings::ContainsIgnoreCase(TextureDiagnosticsPresentation::FormatTextureFormat(row.Format), filter)
	    || Strings::ContainsIgnoreCase(TextureDiagnosticsPresentation::FormatTextureKind(row.Kind), filter);
}
