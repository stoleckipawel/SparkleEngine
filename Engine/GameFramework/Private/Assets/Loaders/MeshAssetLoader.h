#pragma once

#include "Assets/Cooked/LoadedMeshAsset.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <span>

namespace Assets
{
	class MeshAssetLoader final
	{
	  public:
		bool Decode(
		    const std::filesystem::path& path,
		    std::span<const std::uint8_t> bytes,
		    LoadedMeshAsset& outMeshAsset,
		    std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t vertexStride, std::uint32_t indexStride) noexcept;
	};
}
