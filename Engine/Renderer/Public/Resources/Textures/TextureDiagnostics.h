#pragma once

#include "../../../../RHI/Public/Resources/TextureTypes.h"
#include "../../RendererAPI.h"

#include <cstdint>
#include <string>
#include <vector>

enum class TextureDiagnosticsKind : std::uint8_t
{
	DefaultSlot,
	DefaultPath,
	Scene,
};

enum class TextureDiagnosticsResidencyState : std::uint8_t
{
	Unloaded,
	Resident,
};

struct SPARKLE_RENDERER_API TextureDiagnosticsRow final
{
	std::string Key;
	TextureDiagnosticsKind Kind = TextureDiagnosticsKind::Scene;
	TextureResourceDimension Dimension = TextureResourceDimension::Texture2D;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;
	TextureDiagnosticsResidencyState ResidencyState = TextureDiagnosticsResidencyState::Unloaded;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t ArraySize = 0;
	std::string Format;
	std::uint16_t MipCount = 0;
	std::uint64_t EstimatedByteSize = 0;
	std::uint64_t GpuShaderResourceViewId = 0;
	bool Loaded = false;
	bool StreamManaged = false;
};

struct SPARKLE_RENDERER_API TextureDiagnosticsSnapshot final
{
	std::vector<TextureDiagnosticsRow> Rows;
};
