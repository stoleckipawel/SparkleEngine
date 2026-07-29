#include "PCH.h"

#include "Fbx/FbxSkinImporter.h"

#include "Fbx/FbxNodeTransformConverter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class FbxSkinTranslation final
{
  public:
	using NodeSet = std::unordered_set<const aiNode*>;
	using NodeIndexMap = std::unordered_map<const aiNode*, std::uint32_t>;

	static const aiNode* FindCommonAncestor(const std::vector<const aiNode*>& nodes) noexcept
	{
		if (nodes.empty())
		{
			return nullptr;
		}

		for (const aiNode* candidate = nodes.front(); candidate != nullptr; candidate = candidate->mParent)
		{
			bool containsAllNodes = true;
			for (const aiNode* node : nodes)
			{
				const aiNode* ancestor = node;
				while (ancestor != nullptr && ancestor != candidate)
				{
					ancestor = ancestor->mParent;
				}
				if (ancestor == nullptr)
				{
					containsAllNodes = false;
					break;
				}
			}
			if (containsAllNodes)
			{
				return candidate;
			}
		}
		return nullptr;
	}

	static void CollectRequiredNodes(const aiNode& skeletonRoot, const std::vector<const aiNode*>& boneNodes, NodeSet& outRequiredNodes)
	{
		for (const aiNode* boneNode : boneNodes)
		{
			for (const aiNode* node = boneNode; node != nullptr; node = node->mParent)
			{
				outRequiredNodes.insert(node);
				if (node == &skeletonRoot)
				{
					break;
				}
			}
		}
	}

	static void AppendRequiredNodesDepthFirst(const aiNode& node, const NodeSet& requiredNodes, std::vector<const aiNode*>& outNodes)
	{
		if (requiredNodes.contains(&node))
		{
			outNodes.push_back(&node);
		}

		for (unsigned int childIndex = 0; childIndex < node.mNumChildren; ++childIndex)
		{
			AppendRequiredNodesDepthFirst(*node.mChildren[childIndex], requiredNodes, outNodes);
		}
	}

	static const aiBone* FindBoneForNode(const aiMesh& mesh, const aiNode& node) noexcept
	{
		for (unsigned int boneIndex = 0; boneIndex < mesh.mNumBones; ++boneIndex)
		{
			const aiBone* bone = mesh.mBones[boneIndex];
			if (bone != nullptr && bone->mName == node.mName)
			{
				return bone;
			}
		}
		return nullptr;
	}

	static DirectX::XMMATRIX ConvertInverse(const aiMatrix4x4& source)
	{
		const float determinant = source.Determinant();
		if (std::abs(determinant) <= 1.0e-8f)
		{
			throw Diagnostics::Error("FBX skin contains a non-invertible bind transform.");
		}

		aiMatrix4x4 inverse = source;
		inverse.Inverse();
		return FbxNodeTransformConverter::ConvertAssimpMatrixToEngine(inverse);
	}

	static std::string MeshName(const aiMesh& mesh)
	{
		return mesh.mName.length > 0 ? std::string(mesh.mName.C_Str()) : std::string("<unnamed-mesh>");
	}
};

struct FbxSkinImporter::SkeletonBuildState final
{
	std::vector<const aiNode*> BoneNodes;
	std::vector<const aiNode*> JointNodes;
	FbxSkinTranslation::NodeIndexMap JointIndices;
	const aiNode* SkeletonRoot = nullptr;
	DirectX::XMMATRIX BindSpaceCorrection = DirectX::XMMatrixIdentity();
	ImportedSkeleton Skeleton;
};

struct FbxSkinImporter::InfluenceBuildState final
{
	using WeightedJoint = std::pair<std::uint16_t, float>;

	std::unordered_map<std::string, std::uint16_t> JointIndices;
	std::vector<std::vector<WeightedJoint>> VertexWeights;
};

void FbxSkinImporter::CollectSkeletonTopology(
    const aiScene& scene,
    const aiNode& meshNode,
    const aiMesh& mesh,
    SkeletonBuildState& state)
{
	state.BoneNodes.reserve(mesh.mNumBones);
	std::unordered_set<std::string> boneNames;
	for (unsigned int boneIndex = 0; boneIndex < mesh.mNumBones; ++boneIndex)
	{
		const aiBone* bone = mesh.mBones[boneIndex];
		const aiNode* boneNode = bone != nullptr ? FbxNodeTransformConverter::FindNode(scene, bone->mName) : nullptr;
		if (bone == nullptr || bone->mName.length == 0 || boneNode == nullptr || !boneNames.emplace(bone->mName.C_Str()).second)
		{
			throw Diagnostics::Error(
			    std::format("FBX mesh '{}' has an invalid or ambiguous bone {}.", FbxSkinTranslation::MeshName(mesh), boneIndex));
		}
		state.BoneNodes.push_back(boneNode);
	}

	std::vector<const aiNode*> skeletonMembers = state.BoneNodes;
	skeletonMembers.push_back(&meshNode);
	state.SkeletonRoot = FbxSkinTranslation::FindCommonAncestor(skeletonMembers);
	if (state.SkeletonRoot == nullptr)
	{
		throw Diagnostics::Error(std::format("FBX mesh '{}' has no common skeleton root.", FbxSkinTranslation::MeshName(mesh)));
	}

	FbxSkinTranslation::NodeSet requiredNodes;
	FbxSkinTranslation::CollectRequiredNodes(*state.SkeletonRoot, state.BoneNodes, requiredNodes);
	state.JointNodes.reserve(requiredNodes.size());
	FbxSkinTranslation::AppendRequiredNodesDepthFirst(*state.SkeletonRoot, requiredNodes, state.JointNodes);
	if (state.JointNodes.empty() || state.JointNodes.size() > static_cast<std::size_t>((std::numeric_limits<std::uint16_t>::max)()) + 1u)
	{
		throw Diagnostics::Error(std::format(
		    "FBX mesh '{}' has a skeleton joint count outside the engine range: {}.",
		    FbxSkinTranslation::MeshName(mesh),
		    state.JointNodes.size()));
	}

	state.JointIndices.reserve(state.JointNodes.size());
	for (std::size_t jointIndex = 0; jointIndex < state.JointNodes.size(); ++jointIndex)
	{
		state.JointIndices.emplace(state.JointNodes[jointIndex], static_cast<std::uint32_t>(jointIndex));
	}
}

void FbxSkinImporter::InitializeSkeleton(
    const aiScene& scene,
    const aiMesh& mesh,
    std::uint32_t sourceMeshIndex,
    SkeletonBuildState& state)
{
	state.Skeleton.name = state.SkeletonRoot->mName.C_Str();
	state.Skeleton.sourceSkinIndex = sourceMeshIndex;
	state.Skeleton.sourceSkeletonRootNodeIndex = FbxNodeTransformConverter::FindNodeIndex(scene, *state.SkeletonRoot);
	if (state.Skeleton.sourceSkeletonRootNodeIndex == (std::numeric_limits<std::uint32_t>::max)())
	{
		throw Diagnostics::Error(
		    std::format("FBX mesh '{}' skeleton root is outside the imported hierarchy.", FbxSkinTranslation::MeshName(mesh)));
	}
	state.Skeleton.joints.reserve(state.JointNodes.size());

	const aiBone& referenceBone = *mesh.mBones[0];
	const aiMatrix4x4 referenceNodeWorld = FbxNodeTransformConverter::ComputeNodeWorldTransform(*state.BoneNodes[0]);
	state.BindSpaceCorrection =
	    FbxSkinTranslation::ConvertInverse(referenceNodeWorld) * FbxSkinTranslation::ConvertInverse(referenceBone.mOffsetMatrix);
}

void FbxSkinImporter::AppendSkeletonJoints(const aiScene& scene, const aiMesh& mesh, SkeletonBuildState& state)
{
	for (const aiNode* jointNode : state.JointNodes)
	{
		const DirectX::XMMATRIX bindWorld =
		    FbxNodeTransformConverter::ConvertAssimpMatrixToEngine(FbxNodeTransformConverter::ComputeNodeWorldTransform(*jointNode)) *
		    state.BindSpaceCorrection;
		ImportedJoint joint;
		joint.name = jointNode->mName.C_Str();
		joint.sourceNodeIndex = FbxNodeTransformConverter::FindNodeIndex(scene, *jointNode);
		if (joint.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)())
		{
			throw Diagnostics::Error(std::format("FBX skeleton joint '{}' is outside the imported hierarchy.", joint.name));
		}
		if (jointNode != state.SkeletonRoot)
		{
			const auto parent = state.JointIndices.find(jointNode->mParent);
			if (parent == state.JointIndices.end())
			{
				throw Diagnostics::Error(std::format("FBX skeleton joint '{}' has no imported parent.", joint.name));
			}
			joint.parentJointIndex = parent->second;
		}

		DirectX::XMStoreFloat4x4(&joint.bindPoseWorldTransform, bindWorld);
		if (const aiBone* bone = FbxSkinTranslation::FindBoneForNode(mesh, *jointNode))
		{
			const DirectX::XMMATRIX inverseBind = FbxNodeTransformConverter::ConvertAssimpMatrixToEngine(bone->mOffsetMatrix);
			DirectX::XMFLOAT4X4 expectedBind;
			DirectX::XMStoreFloat4x4(&expectedBind, FbxSkinTranslation::ConvertInverse(bone->mOffsetMatrix));
			DirectX::XMStoreFloat4x4(&joint.inverseBindMatrix, inverseBind);
			joint.bindPoseWorldTransform = expectedBind;
		}

		state.Skeleton.joints.push_back(std::move(joint));
	}
}

ImportedSkeletonIndex FbxSkinImporter::ImportSkeleton(
    const aiScene& scene,
    const aiNode& meshNode,
    const aiMesh& mesh,
    std::uint32_t sourceMeshIndex,
    SourceImportOutput& output)
{
	if (!mesh.HasBones())
	{
		return kInvalidImportedSkeletonIndex;
	}

	for (std::size_t skeletonIndex = 0; skeletonIndex < output.scene.skeletons.size(); ++skeletonIndex)
	{
		if (output.scene.skeletons[skeletonIndex].sourceSkinIndex == sourceMeshIndex)
		{
			return static_cast<ImportedSkeletonIndex>(skeletonIndex);
		}
	}

	SkeletonBuildState state;
	CollectSkeletonTopology(scene, meshNode, mesh, state);
	InitializeSkeleton(scene, mesh, sourceMeshIndex, state);
	AppendSkeletonJoints(scene, mesh, state);

	const ImportedSkeletonIndex skeletonIndex = static_cast<ImportedSkeletonIndex>(output.scene.skeletons.size());
	output.scene.skeletons.push_back(std::move(state.Skeleton));
	return skeletonIndex;
}

void FbxSkinImporter::CollectSkinWeights(
    const aiMesh& mesh,
    const ImportedSkeleton& skeleton,
    InfluenceBuildState& state)
{
	state.JointIndices.reserve(skeleton.joints.size());
	for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
	{
		state.JointIndices.emplace(skeleton.joints[jointIndex].name, static_cast<std::uint16_t>(jointIndex));
	}

	state.VertexWeights.resize(mesh.mNumVertices);
	for (unsigned int boneIndex = 0; boneIndex < mesh.mNumBones; ++boneIndex)
	{
		const aiBone* bone = mesh.mBones[boneIndex];
		const auto joint = bone != nullptr ? state.JointIndices.find(bone->mName.C_Str()) : state.JointIndices.end();
		if (bone == nullptr || joint == state.JointIndices.end())
		{
			throw Diagnostics::Error(
			    std::format("FBX mesh '{}' has a bone absent from its imported skeleton.", FbxSkinTranslation::MeshName(mesh)));
		}

		for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
		{
			const aiVertexWeight& sourceWeight = bone->mWeights[weightIndex];
			if (sourceWeight.mVertexId >= mesh.mNumVertices || sourceWeight.mWeight < 0.0f)
			{
				throw Diagnostics::Error(std::format(
				    "FBX mesh '{}' has an invalid weight for bone '{}'.",
				    FbxSkinTranslation::MeshName(mesh),
				    bone->mName.C_Str()));
			}
			if (sourceWeight.mWeight > 0.0f)
			{
				state.VertexWeights[sourceWeight.mVertexId].emplace_back(joint->second, sourceWeight.mWeight);
			}
		}
	}
}

void FbxSkinImporter::WriteSkinInfluences(
    const aiMesh& mesh,
    ImportedMeshGeometry& geometry,
    const InfluenceBuildState& state)
{
	geometry.deformation.skinInfluences.resize(mesh.mNumVertices);
	for (std::size_t vertexIndex = 0; vertexIndex < state.VertexWeights.size(); ++vertexIndex)
	{
		const std::vector<InfluenceBuildState::WeightedJoint>& weights = state.VertexWeights[vertexIndex];
		if (weights.empty() || weights.size() > 8u)
		{
			throw Diagnostics::Error(std::format(
			    "FBX mesh '{}' vertex {} has {} skin influences; the engine supports one to eight without truncation.",
			    FbxSkinTranslation::MeshName(mesh),
			    vertexIndex,
			    weights.size()));
		}

		float weightSum = 0.0f;
		for (const InfluenceBuildState::WeightedJoint& weight : weights)
		{
			weightSum += weight.second;
		}
		if (weightSum <= 1.0e-8f)
		{
			throw Diagnostics::Error(
			    std::format("FBX mesh '{}' vertex {} has zero total skin weight.", FbxSkinTranslation::MeshName(mesh), vertexIndex));
		}

		ImportedSkinInfluence& influence = geometry.deformation.skinInfluences[vertexIndex];
		for (std::size_t influenceIndex = 0; influenceIndex < weights.size(); ++influenceIndex)
		{
			influence.jointIndices[influenceIndex] = weights[influenceIndex].first;
			influence.jointWeights[influenceIndex] = weights[influenceIndex].second / weightSum;
		}
	}
}

void FbxSkinImporter::ImportSkinInfluences(
    const aiMesh& mesh,
    const ImportedSkeleton& skeleton,
    ImportedMeshGeometry& geometry)
{
	if (!mesh.HasBones())
	{
		return;
	}
	if (geometry.vertices.size() != mesh.mNumVertices)
	{
		throw Diagnostics::Error(std::format(
		    "FBX mesh '{}' vertex count differs between geometry and skin data.",
		    FbxSkinTranslation::MeshName(mesh)));
	}

	InfluenceBuildState state;
	CollectSkinWeights(mesh, skeleton, state);
	WriteSkinInfluences(mesh, geometry, state);
}
