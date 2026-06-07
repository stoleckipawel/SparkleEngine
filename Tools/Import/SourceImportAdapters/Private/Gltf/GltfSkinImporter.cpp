#include "PCH.h"

#include "Gltf/GltfSkinImporter.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfNodeTransformUtils.h"

#include <cgltf.h>

#include <cstdint>
#include <format>
#include <limits>
#include <utility>

namespace
{
	std::uint32_t FindJointParentIndex(const cgltf_skin* skin, const cgltf_node* jointNode) noexcept
	{
		if (skin == nullptr || jointNode == nullptr || jointNode->parent == nullptr)
		{
			return (std::numeric_limits<std::uint32_t>::max)();
		}

		for (cgltf_size jointIndex = 0; jointIndex < skin->joints_count; ++jointIndex)
		{
			if (skin->joints[jointIndex] == jointNode->parent)
			{
				return static_cast<std::uint32_t>(jointIndex);
			}
		}

		return (std::numeric_limits<std::uint32_t>::max)();
	}
}  // namespace

ImportedSkinInfluence GltfSkinImporter::ReadSkinInfluence(
    const cgltf_accessor* joints,
    const cgltf_accessor* weights,
    std::size_t vertexIndex) noexcept
{
	ImportedSkinInfluence influence;
	if (joints == nullptr || weights == nullptr || vertexIndex >= joints->count || vertexIndex >= weights->count)
	{
		return influence;
	}

	cgltf_uint jointValues[4] = {0, 0, 0, 0};
	cgltf_accessor_read_uint(joints, vertexIndex, jointValues, 4);
	cgltf_float weightValues[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	cgltf_accessor_read_float(weights, vertexIndex, weightValues, 4);

	float weightSum = 0.0f;
	for (std::size_t influenceIndex = 0; influenceIndex < 4; ++influenceIndex)
	{
		influence.jointIndices[influenceIndex] = static_cast<std::uint16_t>(
		    (std::min)(jointValues[influenceIndex], static_cast<cgltf_uint>((std::numeric_limits<std::uint16_t>::max)())));
		influence.jointWeights[influenceIndex] = (std::max)(0.0f, weightValues[influenceIndex]);
		weightSum += influence.jointWeights[influenceIndex];
	}

	if (weightSum > 0.0f)
	{
		for (float& weight : influence.jointWeights)
		{
			weight /= weightSum;
		}
	}

	return influence;
}

ImportedSkeletonIndex GltfSkinImporter::ImportSkeleton(const cgltf_data* data, const cgltf_skin* skin, SourceImportResult& result)
{
	if (data == nullptr || skin == nullptr || skin->joints_count == 0)
	{
		return kInvalidImportedSkeletonIndex;
	}

	const std::uint32_t sourceSkinIndex = static_cast<std::uint32_t>(cgltf_skin_index(data, skin));
	for (std::size_t skeletonIndex = 0; skeletonIndex < result.scene.skeletons.size(); ++skeletonIndex)
	{
		if (result.scene.skeletons[skeletonIndex].sourceSkinIndex == sourceSkinIndex)
		{
			return static_cast<ImportedSkeletonIndex>(skeletonIndex);
		}
	}

	ImportedSkeleton skeleton;
	skeleton.name = skin->name != nullptr ? skin->name : std::format("Skin {}", sourceSkinIndex);
	skeleton.sourceSkinIndex = sourceSkinIndex;
	skeleton.sourceSkeletonRootNodeIndex = skin->skeleton != nullptr
	                                           ? static_cast<std::uint32_t>(cgltf_node_index(data, skin->skeleton))
	                                           : (std::numeric_limits<std::uint32_t>::max)();
	skeleton.joints.reserve(skin->joints_count);

	for (cgltf_size jointIndex = 0; jointIndex < skin->joints_count; ++jointIndex)
	{
		const cgltf_node* jointNode = skin->joints[jointIndex];
		ImportedJoint joint;
		joint.name = jointNode != nullptr && jointNode->name != nullptr ? jointNode->name : std::format("Joint {}", jointIndex);
		joint.sourceNodeIndex = jointNode != nullptr ? static_cast<std::uint32_t>(cgltf_node_index(data, jointNode))
		                                             : (std::numeric_limits<std::uint32_t>::max)();
		joint.parentJointIndex = FindJointParentIndex(skin, jointNode);

		const DirectX::XMMATRIX inverseBindMatrix =
		    skin->inverse_bind_matrices != nullptr
		        ? GltfNodeTransformUtils::ConvertGltfMatrixToEngine(GltfAccessorReader::ReadFloat4x4(skin->inverse_bind_matrices, jointIndex))
		        : DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&joint.inverseBindMatrix, inverseBindMatrix);

		const DirectX::XMMATRIX bindPoseWorldTransform =
		    jointNode != nullptr ? GltfNodeTransformUtils::ComputeNodeWorldTransform(jointNode) : DirectX::XMMatrixIdentity();
		DirectX::XMStoreFloat4x4(&joint.bindPoseWorldTransform, bindPoseWorldTransform);
		skeleton.joints.push_back(std::move(joint));
	}

	const ImportedSkeletonIndex skeletonIndex = static_cast<ImportedSkeletonIndex>(result.scene.skeletons.size());
	result.scene.skeletons.push_back(std::move(skeleton));
	return skeletonIndex;
}
