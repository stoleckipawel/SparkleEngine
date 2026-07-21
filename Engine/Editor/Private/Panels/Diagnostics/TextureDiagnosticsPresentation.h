#pragma once

#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

struct ImVec2;

namespace TextureDiagnosticsPresentation
{
	std::string FormatTextureDisplayName(const TextureDiagnosticsRow& row);
	std::string FormatTexturePath(const TextureDiagnosticsRow& row);
	std::optional<std::string> FindAuthoredSourcePath(const TextureDiagnosticsRow& row);
	const char* FormatTextureKind(TextureDiagnosticsKind kind) noexcept;
	const char* FormatTextureDimension(TextureResourceDimension dimension) noexcept;
	const char* FormatTextureIntent(TextureFormatIntent intent) noexcept;
	const char* FormatResidency(TextureDiagnosticsResidencyState state) noexcept;
	const char* FormatTextureFormat(const std::string& format) noexcept;
	std::string FormatExtent(const TextureDiagnosticsRow& row);
	std::size_t CountLoadedTextures(const TextureDiagnosticsSnapshot& snapshot) noexcept;
	std::uint64_t SumEstimatedTextureBytes(const TextureDiagnosticsSnapshot& snapshot) noexcept;
	ImVec2 ComputePreviewImageSize(const ImVec2& availableRegion, std::uint32_t width, std::uint32_t height) noexcept;
}
