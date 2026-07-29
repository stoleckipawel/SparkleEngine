#include "PCH.h"

#include "Gltf/GltfMorphTargetImporter.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfNodeTransformConverter.h"

#include <cgltf.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

class GltfMorphTargetMetadata final
{
  public:
	static const cgltf_accessor* FindMorphAttribute(const cgltf_morph_target& target, cgltf_attribute_type type) noexcept
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

	static std::string BuildMorphTargetName(const cgltf_mesh& mesh, std::size_t targetIndex)
	{
		if (targetIndex < mesh.target_names_count && mesh.target_names[targetIndex] != nullptr)
		{
			return mesh.target_names[targetIndex];
		}

		return {};
	}

	static float ReadMeshDefaultWeight(const cgltf_mesh& mesh, std::size_t targetIndex) noexcept
	{
		return targetIndex < mesh.weights_count && mesh.weights != nullptr ? mesh.weights[targetIndex] : 0.0f;
	}
};

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
		const cgltf_accessor* positions = GltfMorphTargetMetadata::FindMorphAttribute(sourceTarget, cgltf_attribute_type_position);
		const cgltf_accessor* normals = GltfMorphTargetMetadata::FindMorphAttribute(sourceTarget, cgltf_attribute_type_normal);
		const cgltf_accessor* tangents = GltfMorphTargetMetadata::FindMorphAttribute(sourceTarget, cgltf_attribute_type_tangent);
		if (positions == nullptr && normals == nullptr && tangents == nullptr)
		{
			continue;
		}
		if ((positions != nullptr && (positions->count != vertexCount || positions->type != cgltf_type_vec3)) ||
		    (normals != nullptr && (normals->count != vertexCount || normals->type != cgltf_type_vec3)) ||
		    (tangents != nullptr && (tangents->count != vertexCount || tangents->type != cgltf_type_vec3)))
		{
			return {};
		}

		ImportedMorphTarget importedTarget;
		importedTarget.name = GltfMorphTargetMetadata::BuildMorphTargetName(mesh, targetIndex);
		importedTarget.defaultWeight = GltfMorphTargetMetadata::ReadMeshDefaultWeight(mesh, targetIndex);
		importedTarget.deltas.resize(vertexCount);
		for (std::uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			ImportedMorphTargetDelta& delta = importedTarget.deltas[vertexIndex];
			if (positions != nullptr)
			{
				const DirectX::XMFLOAT3 value = GltfAccessorReader::ReadFloat3(positions, vertexIndex);
				delta.position = GltfNodeTransformConverter::ConvertGltfVectorToEngine(value);
			}
			if (normals != nullptr)
			{
				const DirectX::XMFLOAT3 value = GltfAccessorReader::ReadFloat3(normals, vertexIndex);
				delta.normal = GltfNodeTransformConverter::ConvertGltfVectorToEngine(value);
			}
			if (tangents != nullptr)
			{
				const DirectX::XMFLOAT3 value = GltfAccessorReader::ReadFloat3(tangents, vertexIndex);
				delta.tangent = GltfNodeTransformConverter::ConvertGltfVectorToEngine(value);
			}
		}

		morphTargets.push_back(std::move(importedTarget));
	}

	return morphTargets;
}

std::vector<float> GltfMorphTargetImporter::BuildNodeMorphWeights(
    const cgltf_mesh& mesh,
    const float* nodeWeights,
    std::size_t nodeWeightCount,
    std::size_t targetCount)
{
	if ((nodeWeightCount > 0 && nodeWeights == nullptr) || (nodeWeightCount == 0 && mesh.weights_count > 0 && mesh.weights == nullptr) ||
	    (nodeWeightCount > 0 && nodeWeightCount != targetCount) ||
	    (nodeWeightCount == 0 && mesh.weights_count > 0 && mesh.weights_count != targetCount))
	{
		return {};
	}
	std::vector<float> weights(targetCount, 0.0f);
	for (std::size_t weightIndex = 0; weightIndex < targetCount; ++weightIndex)
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
