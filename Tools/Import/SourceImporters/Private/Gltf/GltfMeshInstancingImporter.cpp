#include "PCH.h"

#include "Gltf/GltfMeshInstancingImporter.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfNodeTransformConverter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <cstdint>
#include <format>
#include <limits>

class GltfMeshInstancingContract final
{
  public:
	static void ValidateAccessor(
	    const cgltf_accessor* accessor,
	    cgltf_size expectedCount,
	    cgltf_type expectedType,
	    std::string_view nodeLabel,
	    std::string_view attributeName)
	{
		if (accessor == nullptr)
		{
			return;
		}
		if (accessor->component_type != cgltf_component_type_r_32f || accessor->type != expectedType)
		{
			throw Diagnostics::Error(std::format(
			    "glTF node '{}' has an EXT_mesh_gpu_instancing {} accessor with an unsupported component or type.",
			    nodeLabel,
			    attributeName));
		}
		if (accessor->count != expectedCount)
		{
			throw Diagnostics::Error(std::format(
			    "glTF node '{}' has an EXT_mesh_gpu_instancing {} accessor count that differs from its group.",
			    nodeLabel,
			    attributeName));
		}
	}
};

const cgltf_accessor* GltfMeshInstancingImporter::FindMeshGpuInstancingAttribute(const cgltf_node& node, std::string_view attributeName)
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

GltfMeshGpuInstancingTransforms GltfMeshInstancingImporter::ReadMeshGpuInstancingTransforms(
    const cgltf_node& node,
    std::string_view nodeLabel)
{
	GltfMeshGpuInstancingTransforms transforms;
	transforms.translations = FindMeshGpuInstancingAttribute(node, "TRANSLATION");
	transforms.rotations = FindMeshGpuInstancingAttribute(node, "ROTATION");
	transforms.scales = FindMeshGpuInstancingAttribute(node, "SCALE");
	transforms.matrices = FindMeshGpuInstancingAttribute(node, "MATRIX");

	const cgltf_accessor* countSource = transforms.matrices;
	countSource = countSource != nullptr ? countSource : transforms.translations;
	countSource = countSource != nullptr ? countSource : transforms.rotations;
	countSource = countSource != nullptr ? countSource : transforms.scales;
	if (countSource == nullptr)
	{
		throw Diagnostics::Error(
		    std::format("glTF node '{}' has EXT_mesh_gpu_instancing data without a supported transform attribute.", nodeLabel));
	}

	GltfMeshInstancingContract::ValidateAccessor(
	    transforms.translations,
	    countSource->count,
	    cgltf_type_vec3,
	    nodeLabel,
	    "TRANSLATION");
	GltfMeshInstancingContract::ValidateAccessor(transforms.rotations, countSource->count, cgltf_type_vec4, nodeLabel, "ROTATION");
	GltfMeshInstancingContract::ValidateAccessor(transforms.scales, countSource->count, cgltf_type_vec3, nodeLabel, "SCALE");
	GltfMeshInstancingContract::ValidateAccessor(transforms.matrices, countSource->count, cgltf_type_mat4, nodeLabel, "MATRIX");

	if (countSource->count == 0 || countSource->count > (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error(std::format("glTF node '{}' has an EXT_mesh_gpu_instancing count outside the engine range.", nodeLabel));
	}

	transforms.instanceCount = countSource->count;
	return transforms;
}

DirectX::XMMATRIX GltfMeshInstancingImporter::BuildMeshGpuInstancingTransform(
    const GltfMeshGpuInstancingTransforms& transforms,
    std::size_t instanceIndex)
{
	if (transforms.matrices != nullptr)
	{
		return GltfNodeTransformConverter::ConvertGltfMatrixToEngine(
		    GltfAccessorReader::ReadFloat4x4(transforms.matrices, instanceIndex));
	}

	DirectX::XMFLOAT3 translation = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4 rotation = {0.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
	if (transforms.translations != nullptr)
	{
		translation = GltfAccessorReader::ReadFloat3(transforms.translations, instanceIndex);
	}
	if (transforms.rotations != nullptr)
	{
		rotation = GltfAccessorReader::ReadFloat4(transforms.rotations, instanceIndex);
	}
	if (transforms.scales != nullptr)
	{
		scale = GltfAccessorReader::ReadFloat3(transforms.scales, instanceIndex);
	}

	const DirectX::XMMATRIX authoredTransform = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
	                                            DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation)) *
	                                            DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	return GltfNodeTransformConverter::ConvertGltfMatrixToEngine(authoredTransform);
}
