#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "Panels/Diagnostics/TextureDiagnosticsPresentation.h"

#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Panels/Diagnostics/PanelDiagnosticsFormatting.h"
#include "Panels/Diagnostics/TextureDiagnosticMetadataCatalog.h"

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

namespace TextureDiagnosticsPresentation
{

	std::string FormatTextureDisplayName(const TextureDiagnosticsRow& row)
	{
		if (const std::optional<TextureDiagnosticMetadata> metadata = FindTextureDiagnosticMetadata(row))
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
		if (const std::optional<TextureDiagnosticMetadata> metadata = FindTextureDiagnosticMetadata(row))
		{
			if (!metadata->SourcePath.empty())
			{
				return metadata->SourcePath;
			}
		}

		return row.Key;
	}

	std::optional<std::string> FindAuthoredSourcePath(const TextureDiagnosticsRow& row)
	{
		const std::optional<TextureDiagnosticMetadata> metadata = FindTextureDiagnosticMetadata(row);
		return metadata.has_value() && !metadata->SourcePath.empty()
		           ? std::optional<std::string>(metadata->SourcePath)
		           : std::nullopt;
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

	std::size_t CountLoadedTextures(const TextureDiagnosticsSnapshot& snapshot) noexcept
	{
		std::size_t loadedCount = 0;
		for (const TextureDiagnosticsRow& row : snapshot)
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
		for (const TextureDiagnosticsRow& row : snapshot)
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
