#include "PCH.h"

#include "Gltf/GltfMeshInstancingImporter.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfNodeTransformUtils.h"
#include "SourceImportResult.h"

#include <cgltf.h>

#include <cstdint>
#include <format>
#include <limits>

static const auto g_gltfMeshInstancingImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

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

bool GltfMeshInstancingImporter::TryReadMeshGpuInstancingTransforms(
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
		(void)result;
		SPDLOG_LOGGER_WARN(
		    g_gltfMeshInstancingImporterLogger,
		    "{}",
		    std::format(
		        "GltfImporter: Node '{}' has malformed EXT_mesh_gpu_instancing data ({}) and will import as a regular mesh node",
		        nodeLabel,
		        "no supported transform attributes"));
		return false;
	}

	const auto validateAccessor = [&](const cgltf_accessor* accessor, cgltf_type expectedType, std::string_view attributeName) -> bool {
		if (accessor == nullptr)
		{
			return true;
		}

		if (accessor->component_type != cgltf_component_type_r_32f || accessor->type != expectedType)
		{
			SPDLOG_LOGGER_WARN(
			    g_gltfMeshInstancingImporterLogger,
			    "{}",
			    std::format(
			        "GltfImporter: Node '{}' has malformed EXT_mesh_gpu_instancing data ({} accessor has unsupported component/type) and will import as a regular mesh node",
			        nodeLabel,
			        attributeName));
			return false;
		}

		if (accessor->count != countSource->count)
		{
			SPDLOG_LOGGER_WARN(
			    g_gltfMeshInstancingImporterLogger,
			    "{}",
			    std::format(
			        "GltfImporter: Node '{}' has malformed EXT_mesh_gpu_instancing data ({} accessor count does not match the group) and will import as a regular mesh node",
			        nodeLabel,
			        attributeName));
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
		SPDLOG_LOGGER_WARN(
		    g_gltfMeshInstancingImporterLogger,
		    "{}",
		    std::format(
		        "GltfImporter: Node '{}' has malformed EXT_mesh_gpu_instancing data ({}) and will import as a regular mesh node",
		        nodeLabel,
		        "instance count is outside the supported range"));
		return false;
	}

	outTransforms.instanceCount = countSource->count;
	return true;
}

DirectX::XMMATRIX GltfMeshInstancingImporter::BuildMeshGpuInstancingTransform(
    const GltfMeshGpuInstancingTransforms& transforms,
    std::size_t instanceIndex)
{
	if (transforms.matrices != nullptr)
	{
		return GltfNodeTransformUtils::ConvertGltfMatrixToEngine(GltfAccessorReader::ReadFloat4x4(transforms.matrices, instanceIndex));
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

	const DirectX::XMMATRIX authoredTransform =
	    DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
	    DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation)) *
	    DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	return GltfNodeTransformUtils::ConvertGltfMatrixToEngine(authoredTransform);
}
