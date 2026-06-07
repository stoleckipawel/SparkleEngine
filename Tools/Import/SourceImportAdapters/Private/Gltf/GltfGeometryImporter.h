#pragma once

#include "SourceImportResult.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

struct cgltf_data;
struct cgltf_node;
struct cgltf_primitive;

struct GltfMeshGpuInstancingTransforms;

class GltfGeometryImporter final
{
  public:
	static std::size_t CountImportedMeshInstances(const cgltf_data* data);
	static void ImportGeometry(const cgltf_data* data, SourceImportResult& result);

  private:
	static ImportedMaterialIndex ResolveMaterialIndex(
	    const cgltf_primitive& primitive,
	    const cgltf_data* data,
	    std::string_view primitiveLabel,
	    SourceImportResult& result);
	static void AppendMeshInstance(
	    SourceImportResult& result,
	    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
	    ImportedMaterialIndex materialIndex,
	    DirectX::FXMMATRIX worldTransform,
	    ImportedMeshInstanceGroupIndex groupIndex,
	    ImportedSkeletonIndex skeletonIndex,
	    std::uint32_t sourceNodeIndex,
	    std::string_view sourceNodeName);
	static void AppendMeshGpuInstancingGroup(
	    SourceImportResult& result,
	    const GltfMeshGpuInstancingTransforms& transforms,
	    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
	    ImportedMaterialIndex materialIndex,
	    DirectX::FXMMATRIX nodeWorldTransform,
	    ImportedSkeletonIndex skeletonIndex,
	    std::uint32_t sourceNodeIndex,
	    std::string_view sourceNodeName);
	static ImportedMeshPrimitiveIndex FindImportedPrimitiveIndex(
	    const ImportedScene& scene,
	    std::uint32_t sourceMeshIndex,
	    std::uint32_t sourcePrimitiveIndex) noexcept;
	static std::string BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex);
};
