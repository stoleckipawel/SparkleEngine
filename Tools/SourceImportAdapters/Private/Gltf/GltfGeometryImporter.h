#pragma once

#include "SourceImportResult.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct cgltf_accessor;
struct cgltf_data;
struct cgltf_node;
struct cgltf_primitive;

class GltfGeometryImporter final
{
  public:
	static std::size_t CountImportedMeshInstances(const cgltf_data* data);
	static void ImportGeometry(const cgltf_data* data, SourceImportResult& result);

  private:
	static const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, int type);
	static void ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices);
	static DirectX::XMFLOAT2 ReadFloat2(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMFLOAT3 ReadFloat3(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMFLOAT4 ReadFloat4(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static ImportedMaterialIndex ResolveMaterialIndex(
	    const cgltf_primitive& primitive,
	    const cgltf_data* data,
	    std::string_view primitiveLabel,
	    SourceImportResult& result);
	static ImportedMeshGeometry ExtractMeshGeometry(const cgltf_primitive& primitive);
	static std::string BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex);
};
