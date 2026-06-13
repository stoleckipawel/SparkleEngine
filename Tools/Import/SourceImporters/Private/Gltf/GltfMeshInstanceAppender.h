#pragma once

#include "SourceImportResult.h"
#include "Gltf/GltfMeshInstancingImporter.h"

#include <DirectXMath.h>

#include <cstdint>
#include <span>
#include <string_view>

class GltfMeshInstanceAppender final
{
  public:
	static void AppendMeshInstance(
	    SourceImportResult& result,
	    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
	    ImportedMaterialIndex materialIndex,
	    DirectX::FXMMATRIX worldTransform,
	    ImportedMeshInstanceGroupIndex groupIndex,
	    ImportedSkeletonIndex skeletonIndex,
	    std::uint32_t sourceNodeIndex,
	    std::string_view sourceNodeName,
	    std::span<const float> morphWeights = {});

	static void AppendMeshGpuInstancingGroup(
	    SourceImportResult& result,
	    const GltfMeshGpuInstancingTransforms& transforms,
	    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
	    ImportedMaterialIndex materialIndex,
	    DirectX::FXMMATRIX nodeWorldTransform,
	    ImportedSkeletonIndex skeletonIndex,
	    std::uint32_t sourceNodeIndex,
	    std::string_view sourceNodeName,
	    std::span<const float> morphWeights = {});
};
