#pragma once

#include "SourceImportResult.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

struct cgltf_data;
struct cgltf_node;

struct GltfMeshGpuInstancingTransforms;

class GltfGeometryImporter final
{
  public:
	static std::size_t CountImportedMeshInstances(const cgltf_data* data);
	static void ImportGeometry(const cgltf_data* data, SourceImportResult& result);

 private:
	static ImportedMeshPrimitiveIndex FindImportedPrimitiveIndex(
	    const ImportedScene& scene,
	    std::uint32_t sourceMeshIndex,
	    std::uint32_t sourcePrimitiveIndex) noexcept;
	static std::string BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex);
};
