#include "PCH.h"

#include "Gltf/GltfMorphTargetImporter.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfNodeTransformUtils.h"

#include <cgltf.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace
{
	const cgltf_accessor* FindMorphAttribute(const cgltf_morph_target& target, cgltf_attribute_type type) noexcept
	{
		for (cgltf_size attributeIndex = 0; attributeIndex < target.attributes_count; ++attributeIndex)
		{
			if (target.attributes[attributeIndex].type == type)
			{
				return target.attributes[attributeIndex].data;
			}
		}

		return nullptr;
	}

	std::string BuildMorphTargetName(const cgltf_mesh& mesh, std::size_t targetIndex)
	{
		if (targetIndex < mesh.target_names_count && mesh.target_names[targetIndex] != nullptr)
		{
			return mesh.target_names[targetIndex];
		}

		return "MorphTarget" + std::to_string(targetIndex);
	}

	float ReadMeshDefaultWeight(const cgltf_mesh& mesh, std::size_t targetIndex) noexcept
	{
		return targetIndex < mesh.weights_count && mesh.weights != nullptr ? mesh.weights[targetIndex] : 0.0f;
	}
}

std::vector<ImportedMorphTarget> GltfMorphTargetImporter::ImportMorphTargets(
    const cgltf_mesh& mesh,
    const cgltf_primitive& primitive,
    std::uint32_t vertexCount)
{
	std::vector<ImportedMorphTarget> morphTargets;
	morphTargets.reserve(primitive.targets_count);

	for (cgltf_size targetIndex = 0; targetIndex < primitive.targets_count; ++targetIndex)
	{
		const cgltf_morph_target& sourceTarget = primitive.targets[targetIndex];
		const cgltf_accessor* positions = FindMorphAttribute(sourceTarget, cgltf_attribute_type_position);
		const cgltf_accessor* normals = FindMorphAttribute(sourceTarget, cgltf_attribute_type_normal);
		const cgltf_accessor* tangents = FindMorphAttribute(sourceTarget, cgltf_attribute_type_tangent);
		if (positions == nullptr && normals == nullptr && tangents == nullptr)
		{
			continue;
		}

		ImportedMorphTarget importedTarget;
		importedTarget.name = BuildMorphTargetName(mesh, targetIndex);
		importedTarget.defaultWeight = ReadMeshDefaultWeight(mesh, targetIndex);
		importedTarget.deltas.resize(vertexCount);
		for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			ImportedMorphTargetDelta& delta = importedTarget.deltas[vertexIndex];
			if (positions != nullptr)
			{
				delta.position = GltfNodeTransformUtils::ConvertGltfVectorToEngine(GltfAccessorReader::ReadFloat3(positions, vertexIndex));
			}
			if (normals != nullptr)
			{
				delta.normal = GltfNodeTransformUtils::ConvertGltfVectorToEngine(GltfAccessorReader::ReadFloat3(normals, vertexIndex));
			}
			if (tangents != nullptr)
			{
				delta.tangent = GltfNodeTransformUtils::ConvertGltfVectorToEngine(GltfAccessorReader::ReadFloat3(tangents, vertexIndex));
			}
		}

		morphTargets.push_back(std::move(importedTarget));
	}

	return morphTargets;
}

std::vector<float> GltfMorphTargetImporter::BuildNodeMorphWeights(
    const cgltf_mesh& mesh,
    const float* nodeWeights,
    std::size_t nodeWeightCount)
{
	const std::size_t weightCount = nodeWeightCount > 0 ? nodeWeightCount : mesh.weights_count;
	std::vector<float> weights(weightCount, 0.0f);
	for (std::size_t weightIndex = 0; weightIndex < weightCount; ++weightIndex)
	{
		if (nodeWeightCount > 0 && nodeWeights != nullptr && weightIndex < nodeWeightCount)
		{
			weights[weightIndex] = nodeWeights[weightIndex];
		}
		else if (mesh.weights != nullptr && weightIndex < mesh.weights_count)
		{
			weights[weightIndex] = mesh.weights[weightIndex];
		}
	}

	return weights;
}
