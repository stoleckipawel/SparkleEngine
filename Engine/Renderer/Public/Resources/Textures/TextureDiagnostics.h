#pragma once

#include "../../../../RHI/Public/Resources/TextureTypes.h"
#include "../../UI/UiTextureHandle.h"
#include "../../RendererAPI.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using TexturePreviewHandleResolver = std::function<UiTextureHandle(std::uint64_t)>;

enum class TextureDiagnosticsKind : std::uint8_t
{
	DefaultSlot,
	DefaultPath,
	Scene,
};

struct SPARKLE_RENDERER_API TextureDiagnosticsRow final
{
	std::string Key;
	TextureDiagnosticsKind Kind = TextureDiagnosticsKind::Scene;
	TextureResourceDimension Dimension = TextureResourceDimension::Texture2D;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t ArraySize = 0;
	std::string Format;
	std::uint16_t MipCount = 0;
	std::uint64_t EstimatedByteSize = 0;
	UiTextureHandle PreviewTexture = {};
	bool StreamManaged = false;
};

using TextureDiagnosticsSnapshot = std::vector<TextureDiagnosticsRow>;
