#pragma once

#include "Assets/Cooked/LoadedMeshAsset.h"

#include <cstdint>
#include <filesystem>
#include <span>

namespace Assets
{
	class MeshAssetLoader final
	{
	public:
		LoadedMeshAsset Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const;
	};
}
