#pragma once

#include <filesystem>
#include <cstdint>
#include <span>

namespace Assets
{
	struct LoadedSkeletonAsset;

	class SkeletonAssetLoader final
	{
	public:
		LoadedSkeletonAsset Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const;
	};
}
