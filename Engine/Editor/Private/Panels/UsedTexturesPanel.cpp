#include "PCH.h"

#include "Panels/UsedTexturesPanel.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <imgui.h>

namespace
{
	struct TextureDisplayMetadata final
	{
		std::string DisplayName;
		std::string SourcePath;
	};

	struct TextureDisplayMetadataCache final
	{
		std::unordered_map<std::uint64_t, TextureDisplayMetadata> ByAssetId;
		std::unordered_map<std::string, TextureDisplayMetadata> ByCookedPath;
	};

	bool EndsWithIgnoreCase(std::string_view value, std::string_view suffix) noexcept
	{
		if (suffix.size() > value.size())
		{
			return false;
		}
		return Strings::EqualsIgnoreCase(value.substr(value.size() - suffix.size()), suffix);
	}

	bool TryReadTextFile(const std::filesystem::path& path, std::string& outText)
	{
		std::ifstream input(path, std::ios::binary | std::ios::ate);
		if (!input)
		{
			return false;
		}

		const std::ifstream::pos_type fileSize = input.tellg();
		if (fileSize == std::ifstream::pos_type(-1))
		{
			return false;
		}

		outText.resize(static_cast<std::size_t>(fileSize));
		input.seekg(0, std::ios::beg);
		if (!outText.empty())
		{
			input.read(outText.data(), static_cast<std::streamsize>(outText.size()));
		}
		return static_cast<bool>(input);
	}

	std::optional<std::filesystem::path> FindLatestTextureCookSummary()
	{
		const std::filesystem::path summaryRoot = Paths::WorkspaceRoot() / "artifacts" / "diagnostics" / "cook" / "Summaries";
		std::error_code errorCode;
		if (!std::filesystem::exists(summaryRoot, errorCode) || errorCode)
		{
			return std::nullopt;
		}

		std::optional<std::filesystem::path> latestPath;
		std::filesystem::file_time_type latestWriteTime{};
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(summaryRoot, errorCode))
		{
			if (errorCode)
			{
				break;
			}
			if (!entry.is_regular_file(errorCode) || errorCode)
			{
				errorCode.clear();
				continue;
			}

			const std::filesystem::path& path = entry.path();
			if (!EndsWithIgnoreCase(path.filename().generic_string(), "-texturecook-summary.json"))
			{
				continue;
			}

			const std::filesystem::file_time_type writeTime = entry.last_write_time(errorCode);
			if (errorCode)
			{
				errorCode.clear();
				continue;
			}

			if (!latestPath || writeTime > latestWriteTime)
			{
				latestPath = path;
				latestWriteTime = writeTime;
			}
		}

		return latestPath;
	}

	std::string_view FindJsonArray(std::string_view document, std::string_view key) noexcept
	{
		std::size_t cursor = Json::FindPropertyValue(document, key);
		if (cursor == std::string_view::npos || cursor >= document.size() || document[cursor] != '[')
		{
			return {};
		}

		const std::size_t arrayStart = cursor;
		int depth = 0;
		bool inString = false;
		bool escaping = false;
		for (; cursor < document.size(); ++cursor)
		{
			const char character = document[cursor];
			if (inString)
			{
				if (escaping)
				{
					escaping = false;
					continue;
				}
				if (character == '\\')
				{
					escaping = true;
					continue;
				}
				if (character == '"')
				{
					inString = false;
				}
				continue;
			}

			if (character == '"')
			{
				inString = true;
				continue;
			}
			if (character == '[')
			{
				++depth;
				continue;
			}
			if (character == ']')
			{
				--depth;
				if (depth == 0)
				{
					return document.substr(arrayStart, cursor - arrayStart + 1u);
				}
			}
		}

		return {};
	}

	void ParseTextureCookSummaryArray(
	    std::string_view arrayText,
	    TextureDisplayMetadataCache& outCache)
	{
		std::size_t cursor = 0;
		while (cursor < arrayText.size())
		{
			const std::size_t objectStart = arrayText.find('{', cursor);
			if (objectStart == std::string_view::npos)
			{
				return;
			}
			const std::size_t objectEnd = arrayText.find('}', objectStart + 1u);
			if (objectEnd == std::string_view::npos)
			{
				return;
			}

			const std::string_view objectText = arrayText.substr(objectStart, objectEnd - objectStart + 1u);
			std::string assetIdText;
			std::string displayName;
			std::string sourcePath;
			std::string outputPath;
			std::uint64_t assetId = 0;
			if (Json::TryReadStringProperty(objectText, "assetId", assetIdText) && Formatting::TryParseHexUInt64(assetIdText, assetId))
			{
				Json::TryReadStringProperty(objectText, "name", displayName);
				Json::TryReadStringProperty(objectText, "source", sourcePath);
				Json::TryReadStringProperty(objectText, "output", outputPath);
				TextureDisplayMetadata metadata{std::move(displayName), std::move(sourcePath)};
				outCache.ByAssetId[assetId] = metadata;
				if (!outputPath.empty())
				{
					const std::filesystem::path cookedPath{outputPath};
					outCache.ByCookedPath[cookedPath.generic_string()] = metadata;
					outCache.ByCookedPath[cookedPath.filename().generic_string()] = std::move(metadata);
				}
			}

			cursor = objectEnd + 1u;
		}
	}

	TextureDisplayMetadataCache LoadTextureDisplayMetadata()
	{
		TextureDisplayMetadataCache cache;
		const std::optional<std::filesystem::path> summaryPath = FindLatestTextureCookSummary();
		if (!summaryPath)
		{
			return cache;
		}

		std::string summaryText;
		if (!TryReadTextFile(*summaryPath, summaryText))
		{
			return cache;
		}

		std::string schema;
		if (!Json::TryReadStringProperty(summaryText, "schema", schema) || schema != "texture-cooker-summary-v1")
		{
			return cache;
		}

		std::string_view requests = FindJsonArray(summaryText, "allRequests");
		if (requests.empty())
		{
			requests = FindJsonArray(summaryText, "topRequests");
		}
		ParseTextureCookSummaryArray(requests, cache);
		return cache;
	}

	std::optional<std::uint64_t> TryParseCookedTextureAssetId(std::string_view key) noexcept
	{
		const std::filesystem::path path{std::string(key)};
		if (!Strings::EqualsIgnoreCase(path.extension().string(), ".stex"))
		{
			return std::nullopt;
		}

		std::uint64_t assetId = 0;
		const std::string stem = path.stem().string();
		if (Formatting::TryParseHexUInt64(stem, assetId))
		{
			return assetId;
		}

		const std::size_t suffixSeparator = stem.find_last_of('_');
		if (suffixSeparator == std::string::npos || suffixSeparator + 1u >= stem.size())
		{
			return std::nullopt;
		}
		if (!Formatting::TryParseHexUInt64(std::string_view(stem).substr(suffixSeparator + 1u), assetId))
		{
			return std::nullopt;
		}
		return assetId;
	}

	const TextureDisplayMetadata* FindTextureDisplayMetadata(const TextureDiagnosticsRow& row)
	{
		static const TextureDisplayMetadataCache metadata = LoadTextureDisplayMetadata();
		const std::filesystem::path rowPath{row.Key};
		if (const auto metadataIt = metadata.ByCookedPath.find(rowPath.generic_string()); metadataIt != metadata.ByCookedPath.end())
		{
			return &metadataIt->second;
		}
		if (const auto metadataIt = metadata.ByCookedPath.find(rowPath.filename().generic_string()); metadataIt != metadata.ByCookedPath.end())
		{
			return &metadataIt->second;
		}

		const std::optional<std::uint64_t> assetId = TryParseCookedTextureAssetId(row.Key);
		if (!assetId)
		{
			return nullptr;
		}

		const auto metadataIt = metadata.ByAssetId.find(*assetId);
		return metadataIt != metadata.ByAssetId.end() ? &metadataIt->second : nullptr;
	}

	std::string FormatTextureDisplayName(const TextureDiagnosticsRow& row)
	{
		if (const TextureDisplayMetadata* metadata = FindTextureDisplayMetadata(row))
		{
			if (!metadata->DisplayName.empty())
			{
				return metadata->DisplayName;
			}
		}

		const std::string filename = std::filesystem::path(row.Key).filename().generic_string();
		if (!filename.empty())
		{
			return filename;
		}

		return row.Key.empty() ? std::string("<unnamed texture>") : row.Key;
	}

	std::string FormatTexturePath(const TextureDiagnosticsRow& row)
	{
		if (const TextureDisplayMetadata* metadata = FindTextureDisplayMetadata(row))
		{
			if (!metadata->SourcePath.empty())
			{
				return metadata->SourcePath;
			}
		}

		return row.Key;
	}

	void DrawWrappedDisabledText(const std::string& text)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
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

	const char* FormatTextureFormat(const std::string& format) noexcept
	{
		return format.empty() ? "Unknown" : format.c_str();
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
	    "%zu texture(s), %zu loaded, %s estimated",
	    m_snapshot.Rows.size(),
	    CountLoadedTextures(m_snapshot),
	    FormatBytes(SumEstimatedTextureBytes(m_snapshot)).c_str());
}

void UsedTexturesPanel::DrawTextureTable(bool disableInteraction)
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
	                                   ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
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
		}
		ImGui::TableNextColumn();
		const std::string sourcePath = FormatTexturePath(row);
		ImGui::TextUnformatted(sourcePath.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatTextureKind(row.Kind));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.Loaded ? "yes" : "no");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatResidency(row.ResidencyState));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(FormatTextureFormat(row.Format));
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

	const std::string displayName = FormatTextureDisplayName(*selectedRow);
	const std::string sourcePath = FormatTexturePath(*selectedRow);
	ImGui::TextUnformatted(displayName.c_str());
	DrawWrappedDisabledText(sourcePath);
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
	const char* textureFormat = FormatTextureFormat(row.Format);

	if (const TextureDisplayMetadata* metadata = FindTextureDisplayMetadata(row); metadata != nullptr && !metadata->SourcePath.empty())
	{
		UiUtil::DrawKeyValueRow("Source", metadata->SourcePath.c_str());
	}
	UiUtil::DrawKeyValueRow("Kind", FormatTextureKind(row.Kind));
	UiUtil::DrawKeyValueRow("Loaded", row.Loaded ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Resident", FormatResidency(row.ResidencyState));
	UiUtil::DrawKeyValueRow("Streamed", row.StreamManaged ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Default", row.Kind == TextureDiagnosticsKind::Scene ? "no" : "yes");
	UiUtil::DrawKeyValueRow("Dimension", FormatTextureDimension(row.Dimension));
	UiUtil::DrawKeyValueRow("Size", extent.c_str());
	UiUtil::DrawKeyValueRow("Array", arraySize.c_str());
	UiUtil::DrawKeyValueRow("Mips", mips.c_str());
	UiUtil::DrawKeyValueRow("Format", textureFormat);
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
	       Strings::ContainsIgnoreCase(FormatTexturePath(row), filter) || Strings::ContainsIgnoreCase(row.Key, filter) ||
	       Strings::ContainsIgnoreCase(FormatTextureFormat(row.Format), filter) ||
	       Strings::ContainsIgnoreCase(FormatTextureKind(row.Kind), filter);
}
