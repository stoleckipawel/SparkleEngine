#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <span>

namespace Assets
{
	struct LoadedAnimationAsset;

	class AnimationAssetLoader final
	{
	  public:
		bool Decode(
		    const std::filesystem::path& path,
		    std::span<const std::uint8_t> bytes,
		    LoadedAnimationAsset& outAnimationAsset,
		    std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t channelStride, std::uint32_t keyframeStride) noexcept;
	};
}
