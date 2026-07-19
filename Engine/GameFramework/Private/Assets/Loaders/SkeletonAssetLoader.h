#pragma once

#include <filesystem>
#include <cstdint>
#include <string>
#include <span>

namespace Assets
{
	struct LoadedSkeletonAsset;

	class SkeletonAssetLoader final
	{
	  public:
		bool Decode(
		    const std::filesystem::path& path,
		    std::span<const std::uint8_t> bytes,
		    LoadedSkeletonAsset& outSkeletonAsset,
		    std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t jointStride) noexcept;
	};
}
