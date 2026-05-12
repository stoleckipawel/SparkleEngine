#include "PCH.h"

#include "Panels/UsedTexturesPanel.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>

#include <dxgi1_6.h>
#include <imgui.h>

namespace
{
	constexpr std::array<const char*, 4> kPreviewModes = {"Color", "Alpha", "Normal", "Single Channel"};

	std::string FormatTextureDisplayName(const TextureDiagnosticsRow& row)
	{
		const std::string filename = std::filesystem::path(row.Key).filename().generic_string();
		if (!filename.empty())
		{
			return filename;
		}

		return row.Key.empty() ? std::string("<unnamed texture>") : row.Key;
	}

	std::string FormatTexturePath(const TextureDiagnosticsRow& row)
	{
		return row.Key;
	}

	const char* FormatTextureKind(TextureDiagnosticsKind kind) noexcept
	{
		switch (kind)
		{
			case TextureDiagnosticsKind::DefaultSlot:
				return "Default Slot";
			case TextureDiagnosticsKind::DefaultPath:
				return "Default/Fallback";
			case TextureDiagnosticsKind::Scene:
				return "Scene";
			default:
				return "Unknown";
		}
	}

	const char* FormatTextureDimension(TextureResourceDimension dimension) noexcept
	{
		switch (dimension)
		{
			case TextureResourceDimension::TextureCube:
				return "TextureCube";
			case TextureResourceDimension::Texture2D:
			default:
				return "Texture2D";
		}
	}

	const char* FormatTextureIntent(TextureFormatIntent intent) noexcept
	{
		switch (intent)
		{
			case TextureFormatIntent::ColorSrgb:
				return "Color sRGB";
			case TextureFormatIntent::DataLinear:
				return "Data Linear";
			case TextureFormatIntent::Unknown:
			default:
				return "Unknown";
		}
	}

	const char* FormatResidency(TextureDiagnosticsResidencyState state) noexcept
	{
		switch (state)
		{
			case TextureDiagnosticsResidencyState::Resident:
				return "Resident";
			case TextureDiagnosticsResidencyState::Unloaded:
			default:
				return "Unloaded";
		}
	}

	const char* FormatDxgiFormat(std::uint32_t format) noexcept
	{
		switch (static_cast<DXGI_FORMAT>(format))
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				return "R8G8B8A8_UNORM";
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				return "R8G8B8A8_UNORM_SRGB";
			case DXGI_FORMAT_B8G8R8A8_UNORM:
				return "B8G8R8A8_UNORM";
			case DXGI_FORMAT_BC1_UNORM:
				return "BC1_UNORM";
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				return "BC1_UNORM_SRGB";
			case DXGI_FORMAT_BC2_UNORM:
				return "BC2_UNORM";
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				return "BC2_UNORM_SRGB";
			case DXGI_FORMAT_BC3_UNORM:
				return "BC3_UNORM";
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				return "BC3_UNORM_SRGB";
			case DXGI_FORMAT_BC4_UNORM:
				return "BC4_UNORM";
			case DXGI_FORMAT_BC5_UNORM:
				return "BC5_UNORM";
			case DXGI_FORMAT_BC6H_UF16:
				return "BC6H_UF16";
			case DXGI_FORMAT_BC7_UNORM:
				return "BC7_UNORM";
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return "BC7_UNORM_SRGB";
			case DXGI_FORMAT_UNKNOWN:
			default:
				return "Unknown";
		}
	}

	std::string FormatExtent(const TextureDiagnosticsRow& row)
	{
		return std::format("{} x {}", row.Width, row.Height);
	}

	std::string FormatBytes(std::uint64_t bytes)
	{
		constexpr double KiB = 1024.0;
		constexpr double MiB = KiB * 1024.0;
		constexpr double GiB = MiB * 1024.0;
		if (bytes >= static_cast<std::uint64_t>(GiB))
		{
			return std::format("{:.2f} GiB", static_cast<double>(bytes) / GiB);
		}
		if (bytes >= static_cast<std::uint64_t>(MiB))
		{
			return std::format("{:.2f} MiB", static_cast<double>(bytes) / MiB);
		}
		if (bytes >= static_cast<std::uint64_t>(KiB))
		{
			return std::format("{:.1f} KiB", static_cast<double>(bytes) / KiB);
		}
		return std::format("{} B", bytes);
	}

	std::size_t CountLoadedTextures(const TextureDiagnosticsSnapshot& snapshot) noexcept
	{
		std::size_t loadedCount = 0;
		for (const TextureDiagnosticsRow& row : snapshot.Rows)
		{
			if (row.Loaded)
			{
				++loadedCount;
			}
		}
		return loadedCount;
	}

	std::uint64_t SumEstimatedTextureBytes(const TextureDiagnosticsSnapshot& snapshot) noexcept
	{
		std::uint64_t byteCount = 0;
		for (const TextureDiagnosticsRow& row : snapshot.Rows)
		{
			byteCount += row.EstimatedByteSize;
		}
		return byteCount;
	}

	ImVec2 ComputePreviewImageSize(const ImVec2& availableRegion, std::uint32_t width, std::uint32_t height) noexcept
	{
		if (width == 0 || height == 0 || availableRegion.x <= 0.0f || availableRegion.y <= 0.0f)
		{
			return ImVec2(0.0f, 0.0f);
		}

		const float textureWidth = static_cast<float>(width);
		const float textureHeight = static_cast<float>(height);
		const float scale = (std::min) (availableRegion.x / textureWidth, availableRegion.y / textureHeight);
		return ImVec2(textureWidth * scale, textureHeight * scale);
	}
}

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
		ImGui::BeginChild("##TexturePreviewPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
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
	    "%zu texture(s), %zu loaded, %s estimated",
	    m_snapshot.Rows.size(),
	    CountLoadedTextures(m_snapshot),
	    FormatBytes(SumEstimatedTextureBytes(m_snapshot)).c_str());
}

void UsedTexturesPanel::DrawTextureTable(bool disableInteraction)
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
	                                   ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
	if (!ImGui::BeginTable("##UsedTexturesTable", 8, tableFlags, ImVec2(0.0f, 0.0f)))
	{
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Texture");
	ImGui::TableSetupColumn("Kind");
	ImGui::TableSetupColumn("Loaded");
	ImGui::TableSetupColumn("Resident");
	ImGui::TableSetupColumn("Format");
	ImGui::TableSetupColumn("Size");
	ImGui::TableSetupColumn("Mips");
	ImGui::TableSetupColumn("Memory");
	ImGui::TableHeadersRow();

	ImGui::BeginDisabled(disableInteraction);
	for (const TextureDiagnosticsRow& row : m_snapshot.Rows)
	{
		if (!MatchesFilter(row))
		{
			continue;
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		const bool selected = row.Key == m_selectedKey;
		const std::string displayName = FormatTextureDisplayName(row);
		if (ImGui::Selectable(displayName.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
		{
			m_selectedKey = row.Key;
			m_selectedMip = 0;
		}
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatTextureKind(row.Kind));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.Loaded ? "yes" : "no");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatResidency(row.ResidencyState));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatDxgiFormat(row.DxgiFormat));
		ImGui::TableNextColumn();
		const std::string extent = FormatExtent(row);
		ImGui::TextUnformatted(extent.c_str());
		ImGui::TableNextColumn();
		ImGui::Text("%u", static_cast<unsigned int>(row.MipCount));
		ImGui::TableNextColumn();
		const std::string memory = FormatBytes(row.EstimatedByteSize);
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

	ImGui::TextUnformatted(FormatTextureDisplayName(*selectedRow).c_str());
	ImGui::TextDisabled("%s", FormatTexturePath(*selectedRow).c_str());
	ImGui::Separator();
	DrawPreviewControls(disableInteraction, *selectedRow);
	DrawPreview(*selectedRow);
	ImGui::SeparatorText("Runtime Properties");
	DrawSelectedTextureDetails(*selectedRow);
}

void UsedTexturesPanel::DrawPreviewControls(bool disableInteraction, const TextureDiagnosticsRow& row)
{
	ImGui::BeginDisabled(disableInteraction || row.GpuShaderResourceViewId == 0 || row.Dimension != TextureResourceDimension::Texture2D);
	ImGui::SetNextItemWidth(150.0f);
	ImGui::Combo("Mode", &m_previewModeIndex, kPreviewModes.data(), static_cast<int>(kPreviewModes.size()));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	const int maxMip = row.MipCount > 0 ? static_cast<int>(row.MipCount) - 1 : 0;
	m_selectedMip = std::clamp(m_selectedMip, 0, maxMip);
	ImGui::SliderInt("Mip", &m_selectedMip, 0, maxMip);
	ImGui::Checkbox("R", &m_channelR);
	ImGui::SameLine();
	ImGui::Checkbox("G", &m_channelG);
	ImGui::SameLine();
	ImGui::Checkbox("B", &m_channelB);
	ImGui::SameLine();
	ImGui::Checkbox("A", &m_channelA);
	ImGui::EndDisabled();

	if (m_previewModeIndex != 0 || m_selectedMip != 0 || !m_channelR || !m_channelG || !m_channelB || !m_channelA)
	{
		ImGui::TextDisabled("Preview controls are staged for the texture preview shader; raw selected texture preview is active.");
	}
}

void UsedTexturesPanel::DrawPreview(const TextureDiagnosticsRow& row) const
{
	const float detailsReserveHeight = 168.0f;
	ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	availableRegion.y = (std::max) (96.0f, availableRegion.y - detailsReserveHeight);

	if (row.GpuShaderResourceViewId == 0 || row.Dimension != TextureResourceDimension::Texture2D)
	{
		ImGui::BeginChild("##TexturePreviewUnavailable", ImVec2(0.0f, availableRegion.y), ImGuiChildFlags_Borders);
		ImGui::TextDisabled(row.Dimension == TextureResourceDimension::TextureCube ? "Cubemap preview is not available in v1." : "No preview texture is available.");
		ImGui::EndChild();
		return;
	}

	ImGui::BeginChild("##TexturePreview", ImVec2(0.0f, availableRegion.y), ImGuiChildFlags_Borders);
	const ImVec2 imageSize = ComputePreviewImageSize(ImGui::GetContentRegionAvail(), row.Width, row.Height);
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
	ImGui::Image(static_cast<ImTextureID>(row.GpuShaderResourceViewId), imageSize);
	ImGui::EndChild();
}

void UsedTexturesPanel::DrawSelectedTextureDetails(const TextureDiagnosticsRow& row) const
{
	const std::string extent = FormatExtent(row);
	const std::string memory = FormatBytes(row.EstimatedByteSize);
	const std::string mips = std::to_string(row.MipCount);
	const std::string arraySize = std::to_string(row.ArraySize);
	const std::string dxgiFormat = std::format("{} ({})", FormatDxgiFormat(row.DxgiFormat), row.DxgiFormat);

	UiUtil::DrawKeyValueRow("Kind", FormatTextureKind(row.Kind));
	UiUtil::DrawKeyValueRow("Loaded", row.Loaded ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Resident", FormatResidency(row.ResidencyState));
	UiUtil::DrawKeyValueRow("Streamed", row.StreamManaged ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Default", row.Kind == TextureDiagnosticsKind::Scene ? "no" : "yes");
	UiUtil::DrawKeyValueRow("Dimension", FormatTextureDimension(row.Dimension));
	UiUtil::DrawKeyValueRow("Size", extent.c_str());
	UiUtil::DrawKeyValueRow("Array", arraySize.c_str());
	UiUtil::DrawKeyValueRow("Mips", mips.c_str());
	UiUtil::DrawKeyValueRow("Format", dxgiFormat.c_str());
	UiUtil::DrawKeyValueRow("Intent", FormatTextureIntent(row.FormatIntent));
	UiUtil::DrawKeyValueRow("Memory", memory.c_str());
	UiUtil::DrawKeyValueRow("Key", row.Key.c_str());
}

const TextureDiagnosticsRow* UsedTexturesPanel::GetSelectedRow() const noexcept
{
	for (const TextureDiagnosticsRow& row : m_snapshot.Rows)
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
	return filter.empty() || Strings::ContainsIgnoreCase(FormatTextureDisplayName(row), filter) ||
	       Strings::ContainsIgnoreCase(FormatTexturePath(row), filter) || Strings::ContainsIgnoreCase(FormatDxgiFormat(row.DxgiFormat), filter) ||
	       Strings::ContainsIgnoreCase(FormatTextureKind(row.Kind), filter);
}