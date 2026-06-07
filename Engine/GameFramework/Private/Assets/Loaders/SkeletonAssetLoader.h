#pragma once

#include <filesystem>
#include <cstdint>
#include <string>

namespace Assets
{
	struct LoadedSkeletonAsset;

	class SkeletonAssetLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, LoadedSkeletonAsset& outSkeletonAsset, std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t jointStride) noexcept;
	};
}
