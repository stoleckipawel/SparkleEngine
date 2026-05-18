#include "PCH.h"

#include "Gltf/GltfGeometryImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"

#include <cgltf.h>

#include <cstdint>
#include <format>
std::size_t GltfGeometryImporter::CountImportedMeshInstances(const cgltf_data* data)
{
	std::size_t totalPrimitives = 0;
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.mesh)
		{
			totalPrimitives += node.mesh->primitives_count;
		}
	}

	return totalPrimitives;
}

std::string GltfGeometryImporter::BuildPrimitiveLabel(const cgltf_node& node, std::size_t primitiveIndex)
{
	const std::string nodeName = node.name ? node.name : std::string("<unnamed-node>");
	return std::format("node '{}' primitive {}", nodeName, primitiveIndex);
}

void GltfGeometryImporter::ImportGeometry(const cgltf_data* data, SourceImportResult& result)
{
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (!node.mesh)
		{
			continue;
		}

		const DirectX::XMMATRIX worldTransform = ComputeNodeWorldTransform(&node);

		for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
		{
			const cgltf_primitive& primitive = node.mesh->primitives[primitiveIndex];
			const std::string primitiveLabel = BuildPrimitiveLabel(node, primitiveIndex);
			const std::uint32_t sourceMeshIndex = static_cast<std::uint32_t>(cgltf_mesh_index(data, node.mesh));
			const std::uint32_t sourcePrimitiveIndex = static_cast<std::uint32_t>(primitiveIndex);

			if (primitive.type != cgltf_primitive_type_triangles)
			{
				GltfImportDiagnosticLog::ReportSkippedNonTrianglePrimitive(primitiveLabel, result);
				continue;
			}

			if (primitive.targets_count > 0)
			{
				GltfImportDiagnosticLog::ReportIgnoredMorphTargets(primitiveLabel, result);
			}

			if (primitive.has_draco_mesh_compression)
			{
				GltfImportDiagnosticLog::ReportSkippedDracoPrimitive(primitiveLabel, result);
				continue;
			}

			if (primitive.mappings_count > 0)
			{
				GltfImportDiagnosticLog::ReportIgnoredMaterialVariantMappings(primitiveLabel, result);
			}

			ImportedMeshPrimitiveIndex importedPrimitiveIndex = FindImportedPrimitiveIndex(result.scene, sourceMeshIndex, sourcePrimitiveIndex);
			if (importedPrimitiveIndex == kInvalidImportedMeshPrimitiveIndex)
			{
				ImportedMeshGeometry meshGeometry = ExtractMeshGeometry(primitive);
				if (!meshGeometry.IsValid())
				{
					GltfImportDiagnosticLog::ReportSkippedIncompletePrimitive(primitiveLabel, result);
					continue;
				}

				ImportedMeshPrimitive primitiveEntry;
				primitiveEntry.geometry = std::move(meshGeometry);
				primitiveEntry.displayName = primitiveLabel;
				primitiveEntry.sourceMeshIndex = sourceMeshIndex;
				primitiveEntry.sourcePrimitiveIndex = sourcePrimitiveIndex;
				importedPrimitiveIndex = static_cast<ImportedMeshPrimitiveIndex>(result.scene.meshPrimitives.size());
				result.scene.meshPrimitives.push_back(std::move(primitiveEntry));
			}

			ImportedMeshInstance instanceEntry;
			instanceEntry.primitiveIndex = importedPrimitiveIndex;
			instanceEntry.materialIndex = ResolveMaterialIndex(primitive, data, primitiveLabel, result);
			DirectX::XMStoreFloat4x4(&instanceEntry.worldTransform, worldTransform);
			instanceEntry.sourceNodeIndex = static_cast<std::uint32_t>(nodeIndex);
			instanceEntry.sourceNodeName = node.name ? node.name : std::string();
			result.scene.meshInstances.push_back(std::move(instanceEntry));
		}
	}
}

ImportedMeshPrimitiveIndex GltfGeometryImporter::FindImportedPrimitiveIndex(
	const ImportedScene& scene,
	std::uint32_t sourceMeshIndex,
	std::uint32_t sourcePrimitiveIndex) noexcept
{
	for (std::size_t primitiveIndex = 0; primitiveIndex < scene.meshPrimitives.size(); ++primitiveIndex)
	{
		const ImportedMeshPrimitive& primitive = scene.meshPrimitives[primitiveIndex];
		if (primitive.sourceMeshIndex == sourceMeshIndex && primitive.sourcePrimitiveIndex == sourcePrimitiveIndex)
		{
			return static_cast<ImportedMeshPrimitiveIndex>(primitiveIndex);
		}
	}

	return kInvalidImportedMeshPrimitiveIndex;
}

const cgltf_accessor* GltfGeometryImporter::FindAttribute(const cgltf_primitive& primitive, int type)
{
	for (cgltf_size attributeIndex = 0; attributeIndex < primitive.attributes_count; ++attributeIndex)
	{
		if (primitive.attributes[attributeIndex].type == static_cast<cgltf_attribute_type>(type))
		{
			return primitive.attributes[attributeIndex].data;
		}
	}

	return nullptr;
}

void GltfGeometryImporter::ReadIndices(const cgltf_accessor* accessor, std::vector<std::uint32_t>& outIndices)
{
	if (!accessor)
	{
		return;
	}

	outIndices.resize(accessor->count);
	for (cgltf_size index = 0; index < accessor->count; ++index)
	{
		outIndices[index] = static_cast<std::uint32_t>(cgltf_accessor_read_index(accessor, index));
	}
}

DirectX::XMFLOAT2 GltfGeometryImporter::ReadFloat2(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT2 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(DirectX::XMFLOAT2) / sizeof(float));
	}

	return element;
}

DirectX::XMFLOAT3 GltfGeometryImporter::ReadFloat3(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT3 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(DirectX::XMFLOAT3) / sizeof(float));
	}

	return element;
}

DirectX::XMFLOAT4 GltfGeometryImporter::ReadFloat4(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT4 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(DirectX::XMFLOAT4) / sizeof(float));
	}

	return element;
}

DirectX::XMMATRIX GltfGeometryImporter::ComputeNodeWorldTransform(const cgltf_node* node)
{
	DirectX::XMMATRIX worldTransform = DirectX::XMMatrixIdentity();

	const cgltf_node* nodeChain[64];
	int depth = 0;
	for (const cgltf_node* currentNode = node; currentNode != nullptr && depth < 64; currentNode = currentNode->parent)
	{
		nodeChain[depth++] = currentNode;
	}

	for (int chainIndex = depth - 1; chainIndex >= 0; --chainIndex)
	{
		float localMatrix[16];
		cgltf_node_transform_local(nodeChain[chainIndex], localMatrix);
		const DirectX::XMMATRIX localTransform = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(localMatrix));
		worldTransform = DirectX::XMMatrixMultiply(worldTransform, localTransform);
	}

	return worldTransform;
}

ImportedMaterialIndex GltfGeometryImporter::ResolveMaterialIndex(
	const cgltf_primitive& primitive,
	const cgltf_data* data,
	std::string_view primitiveLabel,
	SourceImportResult& result)
{
	if (!primitive.material || result.scene.materials.empty())
	{
		return kInvalidImportedMaterialIndex;
	}

	const std::uint32_t materialIndex = static_cast<std::uint32_t>(primitive.material - data->materials);
	if (materialIndex < result.scene.materials.size())
	{
		return materialIndex;
	}

	GltfImportDiagnosticLog::ReportInvalidMaterialIndex(primitiveLabel, materialIndex, result);
	return kInvalidImportedMaterialIndex;
}

ImportedMeshGeometry GltfGeometryImporter::ExtractMeshGeometry(const cgltf_primitive& primitive)
{
	const cgltf_accessor* positions = FindAttribute(primitive, cgltf_attribute_type_position);
	const cgltf_accessor* normals = FindAttribute(primitive, cgltf_attribute_type_normal);
	const cgltf_accessor* texcoords = FindAttribute(primitive, cgltf_attribute_type_texcoord);
	const cgltf_accessor* tangents = FindAttribute(primitive, cgltf_attribute_type_tangent);

	if (!positions)
	{
		return {};
	}

	const std::uint32_t vertexCount = static_cast<std::uint32_t>(positions->count);
	ImportedMeshGeometry meshGeometry;
	meshGeometry.Reserve(vertexCount, primitive.indices ? static_cast<std::uint32_t>(primitive.indices->count) : 0);
	meshGeometry.vertices.resize(vertexCount);

	for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		ImportedVertex& vertex = meshGeometry.vertices[vertexIndex];
		vertex.position = ReadFloat3(positions, vertexIndex);
		vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};

		if (normals)
		{
			vertex.normal = ReadFloat3(normals, vertexIndex);
		}

		if (texcoords)
		{
			vertex.uv = ReadFloat2(texcoords, vertexIndex);
		}

		if (tangents)
		{
			vertex.tangent = ReadFloat4(tangents, vertexIndex);
		}
	}

	ReadIndices(primitive.indices, meshGeometry.indices);
	return meshGeometry;
}
