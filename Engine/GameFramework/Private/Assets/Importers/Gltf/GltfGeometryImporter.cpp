#include "PCH.h"

#include "Assets/Importers/Gltf/GltfGeometryImporter.h"

#include <cgltf.h>

#include <format>

using namespace DirectX;

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

void GltfGeometryImporter::ImportGeometry(const cgltf_data* data, SceneImportResult& result)
{
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (!node.mesh)
		{
			continue;
		}

		const XMMATRIX worldTransform = ComputeNodeWorldTransform(&node);

		for (cgltf_size primitiveIndex = 0; primitiveIndex < node.mesh->primitives_count; ++primitiveIndex)
		{
			const cgltf_primitive& primitive = node.mesh->primitives[primitiveIndex];
			const std::string primitiveLabel = BuildPrimitiveLabel(node, primitiveIndex);

			if (primitive.type != cgltf_primitive_type_triangles)
			{
				LOG_WARNING(std::format("GltfImporter: Skipping {} because only triangle primitives are supported", primitiveLabel));
				continue;
			}

			if (primitive.targets_count > 0)
			{
				LOG_WARNING(std::format("GltfImporter: {} contains morph targets which will be ignored", primitiveLabel));
			}

			if (primitive.has_draco_mesh_compression)
			{
				LOG_WARNING(std::format("GltfImporter: Skipping {} because Draco-compressed primitives are not supported yet", primitiveLabel));
				continue;
			}

			if (primitive.mappings_count > 0)
			{
				LOG_WARNING(std::format("GltfImporter: {} contains material variant mappings which will be ignored", primitiveLabel));
			}

			MeshData meshData = ExtractMeshGeometry(primitive);
			if (!meshData.IsValid())
			{
				LOG_WARNING(std::format("GltfImporter: Skipping {} because vertex or index data is incomplete", primitiveLabel));
				continue;
			}

			result.materialHandles.push_back(ResolveMaterialHandle(primitive, data, primitiveLabel, result));
			result.transforms.emplace_back(worldTransform);
			result.meshes.push_back(std::move(meshData));
		}
	}
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

XMFLOAT2 GltfGeometryImporter::ReadFloat2(const cgltf_accessor* accessor, std::size_t index)
{
	XMFLOAT2 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(XMFLOAT2) / sizeof(float));
	}

	return element;
}

XMFLOAT3 GltfGeometryImporter::ReadFloat3(const cgltf_accessor* accessor, std::size_t index)
{
	XMFLOAT3 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(XMFLOAT3) / sizeof(float));
	}

	return element;
}

XMFLOAT4 GltfGeometryImporter::ReadFloat4(const cgltf_accessor* accessor, std::size_t index)
{
	XMFLOAT4 element{};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), sizeof(XMFLOAT4) / sizeof(float));
	}

	return element;
}

XMMATRIX GltfGeometryImporter::ComputeNodeWorldTransform(const cgltf_node* node)
{
	XMMATRIX worldTransform = XMMatrixIdentity();

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
		const XMMATRIX localTransform = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(localMatrix));
		worldTransform = XMMatrixMultiply(worldTransform, localTransform);
	}

	return worldTransform;
}

MaterialHandle GltfGeometryImporter::ResolveMaterialHandle(
	const cgltf_primitive& primitive,
	const cgltf_data* data,
	std::string_view primitiveLabel,
	SceneImportResult& result)
{
	if (!primitive.material || result.materials.empty())
	{
		return MaterialHandle::Invalid();
	}

	const std::uint32_t materialIndex = static_cast<std::uint32_t>(primitive.material - data->materials);
	if (materialIndex < result.materials.size())
	{
		return MaterialHandle(materialIndex);
	}

	LOG_WARNING(std::format(
	    "GltfImporter: {} references invalid material index {} and will use the default material",
	    primitiveLabel,
	    materialIndex));
	return MaterialHandle::Invalid();
}

MeshData GltfGeometryImporter::ExtractMeshGeometry(const cgltf_primitive& primitive)
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
	MeshData meshData;
	meshData.Reserve(vertexCount, primitive.indices ? static_cast<std::uint32_t>(primitive.indices->count) : 0);
	meshData.vertices.resize(vertexCount);

	for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		VertexData& vertex = meshData.vertices[vertexIndex];
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

	ReadIndices(primitive.indices, meshData.indices);
	return meshData;
}