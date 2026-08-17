#include "PCH.h"

#include "Gltf/GltfSkinImporter.h"

#include "Gltf/GltfAccessorReader.h"
#include "Gltf/GltfCoordinateConverter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <cstdint>
#include <cmath>
#include <format>
#include <limits>
#include <unordered_set>
#include <utility>

class GltfSkeletonHierarchy final
{
public:
	static std::uint32_t FindJointParentIndex(const cgltf_skin* skin, const cgltf_node* jointNode) noexcept
	{
		if (skin == nullptr || jointNode == nullptr)
		{
			return (std::numeric_limits<std::uint32_t>::max)();
		}

		for (const cgltf_node* ancestor = jointNode->parent; ancestor != nullptr; ancestor = ancestor->parent)
		{
			for (cgltf_size jointIndex = 0; jointIndex < skin->joints_count; ++jointIndex)
			{
				if (skin->joints[jointIndex] == ancestor)
				{
					return static_cast<std::uint32_t>(jointIndex);
				}
			}
		}

		return (std::numeric_limits<std::uint32_t>::max)();
	}

	static DirectX::XMMATRIX Inverse(DirectX::FXMMATRIX matrix, std::string_view label)
	{
		DirectX::XMVECTOR determinant;
		const DirectX::XMMATRIX inverse = DirectX::XMMatrixInverse(&determinant, matrix);
		const float determinantValue = DirectX::XMVectorGetX(determinant);
		if (!std::isfinite(determinantValue) || std::abs(determinantValue) <= 1.0e-8f)
		{
			throw Diagnostics::Error(std::format("glTF skin has a non-invertible {} transform.", label));
		}
		return inverse;
	}
};

ImportedSkinInfluence GltfSkinImporter::ReadSkinInfluence(
    const cgltf_accessor* joints0,
    const cgltf_accessor* weights0,
    const cgltf_accessor* joints1,
    const cgltf_accessor* weights1,
    std::size_t vertexIndex)
{
	if (joints0 == nullptr || weights0 == nullptr || vertexIndex >= joints0->count || vertexIndex >= weights0->count
	    || ((joints1 == nullptr) != (weights1 == nullptr))
	    || (joints1 != nullptr && (vertexIndex >= joints1->count || vertexIndex >= weights1->count)))
	{
		throw Diagnostics::Error("glTF skin influence accessors are incomplete or have inconsistent counts.");
	}

	cgltf_uint jointValues[8] = {};
	cgltf_float weightValues[8] = {};
	if (!cgltf_accessor_read_uint(joints0, vertexIndex, jointValues, 4)
	    || !cgltf_accessor_read_float(weights0, vertexIndex, weightValues, 4)
	    || (joints1 != nullptr
	        && (!cgltf_accessor_read_uint(joints1, vertexIndex, jointValues + 4, 4)
	            || !cgltf_accessor_read_float(weights1, vertexIndex, weightValues + 4, 4))))
	{
		throw Diagnostics::Error(std::format("Cannot decode glTF skin influence at vertex {}.", vertexIndex));
	}

	ImportedSkinInfluence influence;
	float weightSum = 0.0f;
	for (std::size_t influenceIndex = 0; influenceIndex < 8; ++influenceIndex)
	{
		if (jointValues[influenceIndex] > static_cast<cgltf_uint>((std::numeric_limits<std::uint16_t>::max)())
		    || weightValues[influenceIndex] < 0.0f)
		{
			throw Diagnostics::Error(std::format("glTF skin influence {} at vertex {} is invalid.", influenceIndex, vertexIndex));
		}
		influence.jointIndices[influenceIndex] = static_cast<std::uint16_t>(jointValues[influenceIndex]);
		influence.jointWeights[influenceIndex] = weightValues[influenceIndex];
		weightSum += weightValues[influenceIndex];
	}

	if (weightSum <= 0.0f)
	{
		throw Diagnostics::Error(std::format("glTF skin weights at vertex {} have zero total weight.", vertexIndex));
	}

	for (float& weight : influence.jointWeights)
	{
		weight /= weightSum;
	}
	return influence;
}

DirectX::XMMATRIX GltfSkinImporter::ComputeSkinReferenceToWorldTransform(const cgltf_skin* skin)
{
	if (skin == nullptr || skin->joints_count == 0u || skin->joints == nullptr || skin->joints[0] == nullptr)
	{
		throw Diagnostics::Error("glTF skin has no joint from which to define SkinReferenceSpace.");
	}

	DirectX::XMMATRIX inverseBind = DirectX::XMMatrixIdentity();
	if (skin->inverse_bind_matrices != nullptr)
	{
		inverseBind = GltfAccessorReader::ReadFloat4x4(skin->inverse_bind_matrices, 0u);
	}
	inverseBind = GltfCoordinateConverter::ConvertMatrix(inverseBind);
	const DirectX::XMMATRIX firstJointWorld = GltfCoordinateConverter::ComputeNodeWorldTransform(skin->joints[0]);
	const DirectX::XMMATRIX skinReferenceToWorld = inverseBind * firstJointWorld;

	DirectX::XMVECTOR determinant;
	DirectX::XMMatrixInverse(&determinant, skinReferenceToWorld);
	if (!std::isfinite(DirectX::XMVectorGetX(determinant)) || std::abs(DirectX::XMVectorGetX(determinant)) <= 1.0e-8f)
	{
		throw Diagnostics::Error("glTF skin has a non-invertible SkinReferenceSpace-to-WorldSpace transform.");
	}
	return skinReferenceToWorld;
}

ImportedSkeletonIndex GltfSkinImporter::ImportSkeleton(const cgltf_data* data, const cgltf_skin* skin, SourceImportOutput& output)
{
	if (data == nullptr || skin == nullptr || skin->joints_count == 0 || skin->joints == nullptr
	    || skin->joints_count > static_cast<cgltf_size>((std::numeric_limits<std::uint16_t>::max)()) + 1u
	    || (skin->inverse_bind_matrices != nullptr
	        && (skin->inverse_bind_matrices->count != skin->joints_count || skin->inverse_bind_matrices->type != cgltf_type_mat4)))
	{
		throw Diagnostics::Error("glTF skin has incomplete joints or incompatible inverse-bind matrices.");
	}

	const cgltf_size sourceSkinIndexValue = cgltf_skin_index(data, skin);
	if (sourceSkinIndexValue >= data->skins_count || sourceSkinIndexValue > (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error("glTF skin index is outside the parsed scene.");
	}
	const std::uint32_t sourceSkinIndex = static_cast<std::uint32_t>(sourceSkinIndexValue);
	for (std::size_t skeletonIndex = 0; skeletonIndex < output.scene.skeletons.size(); ++skeletonIndex)
	{
		if (output.scene.skeletons[skeletonIndex].sourceSkinIndex == sourceSkinIndex)
		{
			return static_cast<ImportedSkeletonIndex>(skeletonIndex);
		}
	}

	ImportedSkeleton skeleton;
	skeleton.name = skin->name != nullptr ? skin->name : "";
	skeleton.sourceSkinIndex = sourceSkinIndex;
	skeleton.sourceSkeletonRootNodeIndex = skin->skeleton != nullptr ? static_cast<std::uint32_t>(cgltf_node_index(data, skin->skeleton))
	                                                                 : (std::numeric_limits<std::uint32_t>::max)();
	skeleton.joints.reserve(skin->joints_count);
	const DirectX::XMMATRIX skinReferenceToWorld = ComputeSkinReferenceToWorldTransform(skin);
	const DirectX::XMMATRIX worldToSkinReference = GltfSkeletonHierarchy::Inverse(skinReferenceToWorld, "skin-reference-to-world");

	std::unordered_set<const cgltf_node*> uniqueJointNodes;
	uniqueJointNodes.reserve(skin->joints_count);
	for (cgltf_size jointIndex = 0; jointIndex < skin->joints_count; ++jointIndex)
	{
		const cgltf_node* jointNode = skin->joints[jointIndex];
		const cgltf_size sourceNodeIndex = jointNode != nullptr ? cgltf_node_index(data, jointNode) : data->nodes_count;
		if (jointNode == nullptr || sourceNodeIndex >= data->nodes_count || !uniqueJointNodes.emplace(jointNode).second)
		{
			throw Diagnostics::Error(std::format("glTF skin {} has an invalid or duplicate joint {}.", sourceSkinIndex, jointIndex));
		}

		ImportedJoint joint;
		joint.name = jointNode->name != nullptr ? jointNode->name : "";
		joint.sourceNodeIndex = static_cast<std::uint32_t>(sourceNodeIndex);
		joint.parentJointIndex = GltfSkeletonHierarchy::FindJointParentIndex(skin, jointNode);

		DirectX::XMMATRIX inverseBindMatrix = DirectX::XMMatrixIdentity();
		if (skin->inverse_bind_matrices != nullptr)
		{
			inverseBindMatrix = GltfAccessorReader::ReadFloat4x4(skin->inverse_bind_matrices, jointIndex);
		}
		inverseBindMatrix = GltfCoordinateConverter::ConvertMatrix(inverseBindMatrix);
		DirectX::XMStoreFloat4x4(&joint.inverseBindMatrix, inverseBindMatrix);

		const DirectX::XMMATRIX bindLocalTransform = GltfCoordinateConverter::ComputeNodeLocalTransform(jointNode);
		const DirectX::XMMATRIX bindModelTransform = GltfCoordinateConverter::ComputeNodeWorldTransform(jointNode) * worldToSkinReference;
		DirectX::XMMATRIX collapsedBindLocal = bindModelTransform;
		if (joint.parentJointIndex < skin->joints_count)
		{
			const DirectX::XMMATRIX parentBindModel =
			    GltfCoordinateConverter::ComputeNodeWorldTransform(skin->joints[joint.parentJointIndex]) * worldToSkinReference;
			collapsedBindLocal *= GltfSkeletonHierarchy::Inverse(parentBindModel, "parent bind-model");
		}
		const DirectX::XMMATRIX parentSpaceTransform =
		    GltfSkeletonHierarchy::Inverse(bindLocalTransform, "joint-local bind") * collapsedBindLocal;
		DirectX::XMStoreFloat4x4(&joint.bindLocalTransform, bindLocalTransform);
		DirectX::XMStoreFloat4x4(&joint.parentSpaceTransform, parentSpaceTransform);
		DirectX::XMStoreFloat4x4(&joint.bindModelTransform, bindModelTransform);
		skeleton.joints.push_back(std::move(joint));
	}

	const ImportedSkeletonIndex skeletonIndex = static_cast<ImportedSkeletonIndex>(output.scene.skeletons.size());
	output.scene.skeletons.push_back(std::move(skeleton));
	return skeletonIndex;
}
