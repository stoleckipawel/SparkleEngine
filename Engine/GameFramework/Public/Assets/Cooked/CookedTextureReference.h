#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <cstdint>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedTextureReferenceVersion = 1;

	struct SPARKLE_ENGINE_API CookedTextureReference
	{
		CookedAssetId textureAssetId = InvalidCookedAssetId;
		TextureGroup textureGroup = TextureGroup::Diffuse;
		std::uint8_t reserved[7] = {};
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedTextureReference>, "CookedTextureReference must stay trivially copyable.");