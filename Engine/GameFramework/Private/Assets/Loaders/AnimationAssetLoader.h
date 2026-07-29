#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace Assets
{
	struct LoadedAnimationAsset;

	class AnimationAssetLoader final
	{
	  public:
		LoadedAnimationAsset Decode(
		    const std::filesystem::path& path,
		    std::span<const std::uint8_t> bytes) const;
	};
}
