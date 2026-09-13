#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Core/Public/Assets/TextureProperties.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <string>
#include <type_traits>

namespace Assets
{
	struct SPARKLE_ENGINE_API CookedTextureReferenceRecord
	{
		std::uint32_t texturePathByteCount = 0;
		TextureGroup textureGroup = TextureGroup::Diffuse;
		TextureCoordinateMapping mapping;
	};

	struct SPARKLE_ENGINE_API CookedTextureReference
	{
		std::string texturePath;
		TextureGroup textureGroup = TextureGroup::Diffuse;
		TextureCoordinateMapping mapping;

		bool IsValid() const noexcept { return !texturePath.empty(); }
	};
}

static_assert(
    std::is_trivially_copyable_v<Assets::CookedTextureReferenceRecord>,
    "CookedTextureReferenceRecord must stay trivially copyable.");
static_assert(sizeof(TextureCoordinateMapping) == 36u, "TextureCoordinateMapping is part of the cooked-material ABI.");
static_assert(sizeof(Assets::CookedTextureReferenceRecord) == 44u, "CookedTextureReferenceRecord layout changed unexpectedly.");
