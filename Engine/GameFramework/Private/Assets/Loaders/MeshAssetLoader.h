#pragma once

#include "Assets/Cooked/LoadedMeshAsset.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace Assets
{
	class MeshAssetLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, LoadedMeshAsset& outMeshAsset, std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t vertexStride, std::uint32_t indexStride) noexcept;
	};
}
