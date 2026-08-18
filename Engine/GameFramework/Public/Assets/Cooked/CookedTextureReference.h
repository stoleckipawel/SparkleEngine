#pragma once

#include "Core/Public/Assets/TextureGroup.h"
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
		std::uint8_t reserved[4] = {};
	};

	struct SPARKLE_ENGINE_API CookedTextureReference
	{
		std::string texturePath;
		TextureGroup textureGroup = TextureGroup::Diffuse;

		bool IsValid() const noexcept { return !texturePath.empty(); }
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedTextureReferenceRecord>, "CookedTextureReferenceRecord must stay trivially copyable.");
