#include "PCH.h"

#include "Gltf/GltfGeometryImporter.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"

#include <cgltf.h>

#include <cstdint>
#include <format>
#include <limits>
#include <utility>

struct GltfMeshGpuInstancingTransforms
{
	const cgltf_accessor* translations = nullptr;
	const cgltf_accessor* rotations = nullptr;
	const cgltf_accessor* scales = nullptr;
	const cgltf_accessor* matrices = nullptr;
	std::size_t instanceCount = 0;
};

std::size_t GltfGeometryImporter::CountImportedMeshInstances(const cgltf_data* data)
{
	std::size_t totalPrimitives = 0;
	for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex)
	{
		const cgltf_node& node = data->nodes[nodeIndex];
		if (node.mesh)
		{
			std::size_t meshInstanceCount = 1;
			if (node.has_mesh_gpu_instancing && node.mesh_gpu_instancing.attributes_count > 0)
			{
				const cgltf_accessor* firstAccessor = node.mesh_gpu_instancing.attributes[0].data;
				meshInstanceCount = firstAccessor != nullptr ? firstAccessor->count : 1;
			}

			totalPrimitives += node.mesh->primitives_count * meshInstanceCount;
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
		GltfMeshGpuInstancingTransforms meshGpuInstancingTransforms;
		const std::string nodeLabel = node.name ? node.name : std::format("node {}", nodeIndex);
		const bool hasMeshGpuInstancing = node.has_mesh_gpu_instancing && node.mesh_gpu_instancing.attributes_count > 0;
		const bool importMeshGpuInstancing =
		    hasMeshGpuInstancing && TryReadMeshGpuInstancingTransforms(node, nodeLabel, result, meshGpuInstancingTransforms);

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

			const ImportedMaterialIndex materialIndex = ResolveMaterialIndex(primitive, data, primitiveLabel, result);
			if (importMeshGpuInstancing)
			{
				AppendMeshGpuInstancingGroup(
				    result,
				    meshGpuInstancingTransforms,
				    importedPrimitiveIndex,
				    materialIndex,
				    worldTransform,
				    static_cast<std::uint32_t>(nodeIndex),
				    node.name ? std::string_view(node.name) : std::string_view());
			}
			else
			{
				AppendMeshInstance(
				    result,
				    importedPrimitiveIndex,
				    materialIndex,
				    worldTransform,
				    kInvalidImportedMeshInstanceGroupIndex,
				    static_cast<std::uint32_t>(nodeIndex),
				    node.name ? std::string_view(node.name) : std::string_view());
			}
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

DirectX::XMFLOAT3 GltfGeometryImporter::ConvertGltfVectorToEngine(const DirectX::XMFLOAT3& value) noexcept
{
	// glTF is right-handed (+Y up, +Z forward); the runtime renderer is left-handed.
	return DirectX::XMFLOAT3(value.x, value.y, -value.z);
}

DirectX::XMFLOAT4 GltfGeometryImporter::ConvertGltfTangentToEngine(const DirectX::XMFLOAT4& value) noexcept
{
	// Mirroring Z flips the tangent frame handedness, so tangent.w must flip too.
	return DirectX::XMFLOAT4(value.x, value.y, -value.z, -value.w);
}

DirectX::XMMATRIX GltfGeometryImporter::ConvertGltfMatrixToEngine(DirectX::FXMMATRIX matrix) noexcept
{
	const DirectX::XMMATRIX handedness = DirectX::XMMatrixScaling(1.0f, 1.0f, -1.0f);
	return DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(handedness, matrix), handedness);
}

void GltfGeometryImporter::ConvertGltfTriangleWindingToEngine(std::vector<std::uint32_t>& indices) noexcept
{
	for (std::size_t index = 0; index + 2 < indices.size(); index += 3)
	{
		std::swap(indices[index + 1], indices[index + 2]);
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

DirectX::XMMATRIX GltfGeometryImporter::ReadFloat4x4(const cgltf_accessor* accessor, std::size_t index)
{
	DirectX::XMFLOAT4X4 element = {
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f};
	if (accessor && index < accessor->count)
	{
		cgltf_accessor_read_float(accessor, index, reinterpret_cast<cgltf_float*>(&element), 16);
	}

	return DirectX::XMLoadFloat4x4(&element);
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

	return ConvertGltfMatrixToEngine(worldTransform);
}

const cgltf_accessor* GltfGeometryImporter::FindMeshGpuInstancingAttribute(const cgltf_node& node, std::string_view attributeName)
{
	if (!node.has_mesh_gpu_instancing)
	{
		return nullptr;
	}

	for (cgltf_size attributeIndex = 0; attributeIndex < node.mesh_gpu_instancing.attributes_count; ++attributeIndex)
	{
		const cgltf_attribute& attribute = node.mesh_gpu_instancing.attributes[attributeIndex];
		if (attribute.name != nullptr && attributeName == attribute.name)
		{
			return attribute.data;
		}
	}

	return nullptr;
}

bool GltfGeometryImporter::TryReadMeshGpuInstancingTransforms(
    const cgltf_node& node,
    std::string_view nodeLabel,
    SourceImportResult& result,
    GltfMeshGpuInstancingTransforms& outTransforms)
{
	outTransforms = {};
	outTransforms.translations = FindMeshGpuInstancingAttribute(node, "TRANSLATION");
	outTransforms.rotations = FindMeshGpuInstancingAttribute(node, "ROTATION");
	outTransforms.scales = FindMeshGpuInstancingAttribute(node, "SCALE");
	outTransforms.matrices = FindMeshGpuInstancingAttribute(node, "MATRIX");

	const cgltf_accessor* countSource = outTransforms.matrices;
	countSource = countSource != nullptr ? countSource : outTransforms.translations;
	countSource = countSource != nullptr ? countSource : outTransforms.rotations;
	countSource = countSource != nullptr ? countSource : outTransforms.scales;
	if (countSource == nullptr)
	{
		GltfImportDiagnosticLog::ReportMalformedGpuInstancing(nodeLabel, "no supported transform attributes", result);
		return false;
	}

	const auto validateAccessor = [&](const cgltf_accessor* accessor, cgltf_type expectedType, std::string_view attributeName) -> bool {
		if (accessor == nullptr)
		{
			return true;
		}

		if (accessor->component_type != cgltf_component_type_r_32f || accessor->type != expectedType)
		{
			GltfImportDiagnosticLog::ReportMalformedGpuInstancing(nodeLabel, std::format("{} accessor has unsupported component/type", attributeName), result);
			return false;
		}

		if (accessor->count != countSource->count)
		{
			GltfImportDiagnosticLog::ReportMalformedGpuInstancing(nodeLabel, std::format("{} accessor count does not match the group", attributeName), result);
			return false;
		}

		return true;
	};

	if (!validateAccessor(outTransforms.translations, cgltf_type_vec3, "TRANSLATION") ||
	    !validateAccessor(outTransforms.rotations, cgltf_type_vec4, "ROTATION") ||
	    !validateAccessor(outTransforms.scales, cgltf_type_vec3, "SCALE") ||
	    !validateAccessor(outTransforms.matrices, cgltf_type_mat4, "MATRIX"))
	{
		return false;
	}

	if (countSource->count == 0 || countSource->count > (std::numeric_limits<std::uint32_t>::max)())
	{
		GltfImportDiagnosticLog::ReportMalformedGpuInstancing(nodeLabel, "instance count is outside the supported range", result);
		return false;
	}

	outTransforms.instanceCount = countSource->count;
	return true;
}

DirectX::XMMATRIX GltfGeometryImporter::BuildMeshGpuInstancingTransform(
    const GltfMeshGpuInstancingTransforms& transforms,
    std::size_t instanceIndex)
{
	if (transforms.matrices != nullptr)
	{
		return ConvertGltfMatrixToEngine(ReadFloat4x4(transforms.matrices, instanceIndex));
	}

	DirectX::XMFLOAT3 translation = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
	if (transforms.translations != nullptr)
	{
		translation = ReadFloat3(transforms.translations, instanceIndex);
	}
	if (transforms.rotations != nullptr)
	{
		rotation = ReadFloat4(transforms.rotations, instanceIndex);
	}
	if (transforms.scales != nullptr)
	{
		scale = ReadFloat3(transforms.scales, instanceIndex);
	}

	const DirectX::XMMATRIX authoredTransform =
	    DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
	    DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation)) *
	    DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	return ConvertGltfMatrixToEngine(authoredTransform);
}

void GltfGeometryImporter::AppendMeshInstance(
    SourceImportResult& result,
    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
    ImportedMaterialIndex materialIndex,
    DirectX::FXMMATRIX worldTransform,
    ImportedMeshInstanceGroupIndex groupIndex,
    std::uint32_t sourceNodeIndex,
    std::string_view sourceNodeName)
{
	ImportedMeshInstance instanceEntry;
	instanceEntry.primitiveIndex = importedPrimitiveIndex;
	instanceEntry.materialIndex = materialIndex;
	instanceEntry.groupIndex = groupIndex;
	DirectX::XMStoreFloat4x4(&instanceEntry.worldTransform, worldTransform);
	instanceEntry.sourceNodeIndex = sourceNodeIndex;
	instanceEntry.sourceNodeName = sourceNodeName;
	result.scene.meshInstances.push_back(std::move(instanceEntry));
}

void GltfGeometryImporter::AppendMeshGpuInstancingGroup(
    SourceImportResult& result,
    const GltfMeshGpuInstancingTransforms& transforms,
    ImportedMeshPrimitiveIndex importedPrimitiveIndex,
    ImportedMaterialIndex materialIndex,
    DirectX::FXMMATRIX nodeWorldTransform,
    std::uint32_t sourceNodeIndex,
    std::string_view sourceNodeName)
{
	const ImportedMeshInstanceGroupIndex groupIndex = static_cast<ImportedMeshInstanceGroupIndex>(result.scene.meshInstanceGroups.size());
	const ImportedMeshInstanceIndex firstInstanceIndex = static_cast<ImportedMeshInstanceIndex>(result.scene.meshInstances.size());

	for (std::size_t instanceIndex = 0; instanceIndex < transforms.instanceCount; ++instanceIndex)
	{
		const DirectX::XMMATRIX authoredInstanceTransform = BuildMeshGpuInstancingTransform(transforms, instanceIndex);
		const DirectX::XMMATRIX worldTransform = DirectX::XMMatrixMultiply(nodeWorldTransform, authoredInstanceTransform);
		AppendMeshInstance(
		    result,
		    importedPrimitiveIndex,
		    materialIndex,
		    worldTransform,
		    groupIndex,
		    sourceNodeIndex,
		    sourceNodeName);
	}

	ImportedMeshInstanceGroup groupEntry;
	groupEntry.primitiveIndex = importedPrimitiveIndex;
	groupEntry.materialIndex = materialIndex;
	groupEntry.firstInstanceIndex = firstInstanceIndex;
	groupEntry.instanceCount = static_cast<std::uint32_t>(transforms.instanceCount);
	groupEntry.groupKind = ImportedMeshInstanceGroupKind::AuthoredInstanceGroup;
	result.scene.meshInstanceGroups.push_back(groupEntry);
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
		vertex.position = ConvertGltfVectorToEngine(ReadFloat3(positions, vertexIndex));
		vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};

		if (normals)
		{
			vertex.normal = ConvertGltfVectorToEngine(ReadFloat3(normals, vertexIndex));
		}

		if (texcoords)
		{
			vertex.uv = ReadFloat2(texcoords, vertexIndex);
		}

		if (tangents)
		{
			vertex.tangent = ConvertGltfTangentToEngine(ReadFloat4(tangents, vertexIndex));
		}
	}

	ReadIndices(primitive.indices, meshGeometry.indices);
	ConvertGltfTriangleWindingToEngine(meshGeometry.indices);
	return meshGeometry;
}
