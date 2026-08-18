#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <type_traits>

namespace Assets
{
	using CookedAssetId = std::uint64_t;

	inline constexpr CookedAssetId InvalidCookedAssetId = 0;

	struct SPARKLE_ENGINE_API CookedAssetHeader
	{
		std::uint32_t magic = 0;

		bool HasMagic(std::uint32_t expectedMagic) const noexcept;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedAssetHeader>, "CookedAssetHeader must stay trivially copyable.");
