#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <type_traits>

namespace Engine::Assets
{
	using CookedAssetId = std::uint64_t;

	inline constexpr CookedAssetId InvalidCookedAssetId = 0;

	constexpr std::uint32_t MakeCookedAssetMagic(char a, char b, char c, char d) noexcept
	{
		return static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 8u) |
		       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 16u) |
		       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(d)) << 24u);
	}

	struct SPARKLE_ENGINE_API CookedAssetHeader
	{
		std::uint32_t magic = 0;
		std::uint32_t version = 0;

		constexpr bool Matches(std::uint32_t expectedMagic, std::uint32_t expectedVersion) const noexcept
		{
			return magic == expectedMagic && version == expectedVersion;
		}
	};
}

static_assert(std::is_trivially_copyable_v<Engine::Assets::CookedAssetHeader>, "CookedAssetHeader must stay trivially copyable.");