// =============================================================================
// GltfLoader.h — glTF 2.0 Asset Loader
// =============================================================================
//
// Loads glTF / GLB files into CPU-side mesh and material data.
// Pure CPU work — no GPU operations, no Renderer or RHI dependencies.
//
// USAGE:
//   GltfLoader::LoadResult result = GltfLoader::Load("Sponza/Sponza.gltf");
//   if (result.success)
//   {
//       for (auto& mesh : result.meshes) { ... }
//       for (auto& mat  : result.materials) { ... }
//   }
//
// DESIGN:
//   - Static loader (no instance state needed)
//   - Returns a self-contained LoadResult with all data
//   - Each glTF primitive becomes one MeshData
//   - Transforms are pre-computed to world space per node hierarchy
//   - Materials map 1:1 to glTF PBR metallic-roughness workflow
//
// NOTES:
//   - Uses cgltf for parsing (header-only, C99)
//   - Texture paths are relative to the glTF file directory
//   - Scene stores results; Renderer uploads to GPU on demand
//
// =============================================================================

#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/Scene/MeshData.h"

#include <DirectXMath.h>
#include <filesystem>
#include <string>
#include <vector>

// =============================================================================
// GltfLoader
// =============================================================================

class SPARKLE_ENGINE_API GltfLoader final
{
  public:
	// =========================================================================
	// Load Result
	// =========================================================================

	/// Self-contained result from loading a glTF file.
	/// Owns all loaded data — caller takes ownership via move.
	struct LoadResult
	{
		std::vector<MeshData> meshes;                 // One per glTF primitive
		std::vector<MaterialDesc> materials;          // One per glTF material
		std::vector<std::string> texturePaths;        // Unique texture file paths
		std::vector<DirectX::XMFLOAT4X4> transforms;  // World transform per mesh
		std::vector<std::uint32_t> materialIndices;   // Material index per mesh (into materials[])

		bool bSuccess = false;
		std::string errorMessage;

		bool IsValid() const noexcept { return bSuccess && !meshes.empty(); }
		std::size_t GetMeshCount() const noexcept { return meshes.size(); }
		std::size_t GetMaterialCount() const noexcept { return materials.size(); }
	};

	// =========================================================================
	// Public API
	// =========================================================================

	/// Loads a glTF or GLB file from an absolute path.
	/// @param filePath Absolute filesystem path to the .gltf or .glb file
	/// @return LoadResult containing all meshes, materials, and transforms
	static LoadResult Load(const std::filesystem::path& filePath);

	// Static utility — no instantiation
	GltfLoader() = delete;
	~GltfLoader() = delete;
};
