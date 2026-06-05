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

struct GltfMeshGpuInstancingTransforms;

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
	static DirectX::XMMATRIX ReadFloat4x4(const cgltf_accessor* accessor, std::size_t index);
	static DirectX::XMFLOAT3 ConvertGltfVectorToEngine(const DirectX::XMFLOAT3& value) noexcept;
	static DirectX::XMFLOAT4 ConvertGltfTangentToEngine(const DirectX::XMFLOAT4& value) noexcept;
	static DirectX::XMMATRIX ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept;
	static void ConvertGltfTriangleWindingToEngine(std::vector<std::uint32_t>& indices) noexcept;
	static DirectX::XMMATRIX ComputeNodeWorldTransform(const cgltf_node* node);
	static const cgltf_accessor* FindMeshGpuInstancingAttribute(const cgltf_node& node, std::string_view attributeName);
	static bool TryReadMeshGpuInstancingTransforms(
	    const cgltf_node& node,
	    std::string_view nodeLabel,
	    SourceImportResult& result,
	    GltfMeshGpuInstancingTransforms& outTransforms);
	static DirectX::XMMATRIX BuildMeshGpuInstancingTransform(const GltfMeshGpuInstancingTransforms& transforms, std::size_t instanceIndex);
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
	    std::uint32_t sourceNodeIndex,
	    std::string_view sourceNodeName);
	static void AppendMeshGpuInstancingGroup(
	    SourceImportResult& result,
	    const GltfMeshGpuInstancingTransforms& transforms,
	    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
	    ImportedMaterialIndex materialIndex,
	    DirectX::FXMMATRIX nodeWorldTransform,
	    std::uint32_t sourceNodeIndex,
	    std::string_view sourceNodeName);
	static ImportedMeshGeometry ExtractMeshGeometry(const cgltf_primitive& primitive);
	static ImportedMeshPrimitiveIndex FindImportedPrimitiveIndex(
	    const ImportedScene& scene,
	    std::uint32_t sourceMeshIndex,
	    std::uint32_t sourcePrimitiveIndex) noexcept;
	static std::string BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex);
};
