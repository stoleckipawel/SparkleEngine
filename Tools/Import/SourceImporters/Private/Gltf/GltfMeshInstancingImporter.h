#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <string_view>

struct SourceImportResult;
struct cgltf_accessor;
struct cgltf_node;

struct GltfMeshGpuInstancingTransforms
{
	const cgltf_accessor* translations = nullptr;
	const cgltf_accessor* rotations = nullptr;
	const cgltf_accessor* scales = nullptr;
	const cgltf_accessor* matrices = nullptr;
	std::size_t instanceCount = 0;
};

class GltfMeshInstancingImporter final
{
  public:
	static bool TryReadMeshGpuInstancingTransforms(
	    const cgltf_node& node,
	    std::string_view nodeLabel,
	    SourceImportResult& result,
	    GltfMeshGpuInstancingTransforms& outTransforms);
	static DirectX::XMMATRIX BuildMeshGpuInstancingTransform(
	    const GltfMeshGpuInstancingTransforms& transforms,
	    std::size_t instanceIndex);

  private:
	static const cgltf_accessor* FindMeshGpuInstancingAttribute(const cgltf_node& node, std::string_view attributeName);
};
