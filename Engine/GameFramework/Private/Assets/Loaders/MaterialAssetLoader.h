#pragma once

#include <filesystem>
#include <span>
#include <cstdint>

namespace Assets
{
	struct LoadedMaterialAsset;

	class MaterialAssetLoader final
	{
	public:
		LoadedMaterialAsset Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const;
	};
}
