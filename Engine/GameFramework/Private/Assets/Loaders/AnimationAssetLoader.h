#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace Assets
{
	struct LoadedAnimationAsset;

	class AnimationAssetLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, LoadedAnimationAsset& outAnimationAsset, std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t channelStride, std::uint32_t keyframeStride) noexcept;
	};
}
