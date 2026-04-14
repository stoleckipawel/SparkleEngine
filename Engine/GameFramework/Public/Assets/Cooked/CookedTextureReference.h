#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <cstdint>
#include <type_traits>

namespace Engine::Assets
{
	enum class CookedTextureSemantic : std::uint8_t
	{
		Albedo = 0,
		Normal = 1,
		MetallicRoughness = 2,
		Occlusion = 3,
		Emissive = 4,
	};

	inline constexpr std::uint32_t kCookedTextureReferenceVersion = 1;

	struct SPARKLE_ENGINE_API CookedTextureReference
	{
		CookedAssetId textureAssetId = InvalidCookedAssetId;
		CookedTextureSemantic semantic = CookedTextureSemantic::Albedo;
		std::uint8_t reserved[7] = {};
	};
}

static_assert(std::is_trivially_copyable_v<Engine::Assets::CookedTextureReference>, "CookedTextureReference must stay trivially copyable.");