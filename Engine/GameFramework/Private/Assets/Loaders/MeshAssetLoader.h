#pragma once

#include "Scene/Meshes/MeshData.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace Engine::Assets
{
	class MeshAssetLoader final
	{
	  public:
		bool Load(const std::filesystem::path& path, MeshData& outMeshData, std::string& outErrorMessage) const;

	  private:
		static bool HasValidHeader(std::uint32_t vertexStride, std::uint32_t indexStride) noexcept;
	};
}