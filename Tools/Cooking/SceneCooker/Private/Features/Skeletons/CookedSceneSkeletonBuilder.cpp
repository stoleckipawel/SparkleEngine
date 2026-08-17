#include "PCH.h"

#include "Features/Skeletons/CookedSceneSkeletonBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Hash/HashUtils.h"
#include "GameFramework/Public/Scene/Animations/SkeletonTransformContract.h"

#include <cstring>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class CookedSkeletonTranslation final
{
public:
	static Assets::CookedAssetId BuildSkeletonAssetId(std::string_view sceneAssetId, std::uint32_t sourceSkinIndex)
	{
		return Hash::Fnv1a64(std::string(sceneAssetId) + "#skeleton#" + std::to_string(sourceSkinIndex));
	}

	static void CopyName(std::string_view sourceName, char (&destination)[64]) noexcept
	{
		std::memset(destination, 0, sizeof(destination));
		std::memcpy(destination, sourceName.data(), sourceName.size());
	}
};

void CookedSceneSkeletonBuilder::BuildSkeletons(
    const SourceImportOutput& importOutput,
    std::string_view sceneAssetId,
    CookedSceneBuild& outBuild)
{
	outBuild.outputs.skeletonAssets.clear();
	outBuild.manifest.skeletonRefs.clear();
	outBuild.outputs.skeletonAssets.reserve(importOutput.scene.skeletons.size());
	outBuild.manifest.skeletonRefs.reserve(importOutput.scene.skeletons.size());

	std::unordered_set<std::uint32_t> sourceSkinIndices;
	for (std::size_t skeletonIndex = 0; skeletonIndex < importOutput.scene.skeletons.size(); ++skeletonIndex)
	{
		const ImportedSkeleton& importedSkeleton = importOutput.scene.skeletons[skeletonIndex];
		if (!importedSkeleton.IsValid())
		{
			throw Diagnostics::Error(std::format("Imported skeleton {} has no joints.", skeletonIndex));
		}
		if (!sourceSkinIndices.insert(importedSkeleton.sourceSkinIndex).second)
		{
			throw Diagnostics::Error(
			    std::format("Imported skeleton {} duplicates source skin index {}.", skeletonIndex, importedSkeleton.sourceSkinIndex));
		}
		std::vector<std::uint32_t> evaluationOrder;
		if (!SkeletonTransformContract::BuildEvaluationOrder(importedSkeleton.joints, evaluationOrder))
		{
			throw Diagnostics::Error(std::format("Imported skeleton {} has a cyclic joint hierarchy.", skeletonIndex));
		}

		CookedSkeletonAssetBuild skeletonAsset;
		skeletonAsset.assetId = CookedSkeletonTranslation::BuildSkeletonAssetId(sceneAssetId, importedSkeleton.sourceSkinIndex);
		skeletonAsset.sourceSkinIndex = importedSkeleton.sourceSkinIndex;
		skeletonAsset.sourcePath = importOutput.GetSourcePath();
		skeletonAsset.joints.reserve(importedSkeleton.joints.size());

		std::unordered_set<std::uint32_t> sourceNodeIndices;
		for (std::size_t jointIndex = 0; jointIndex < importedSkeleton.joints.size(); ++jointIndex)
		{
			const ImportedJoint& importedJoint = importedSkeleton.joints[jointIndex];
			if (importedJoint.name.size() >= sizeof(Assets::CookedSkeletonJointRecord::name)
			    || importedJoint.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)()
			    || !sourceNodeIndices.insert(importedJoint.sourceNodeIndex).second
			    || (importedJoint.parentJointIndex != (std::numeric_limits<std::uint32_t>::max)()
			        && (importedJoint.parentJointIndex >= importedSkeleton.joints.size() || importedJoint.parentJointIndex == jointIndex)))
			{
				throw Diagnostics::Error(std::format("Imported skeleton {} has invalid joint {}.", skeletonIndex, jointIndex));
			}
			const DirectX::XMFLOAT4X4* parentBindModel = importedJoint.parentJointIndex < importedSkeleton.joints.size()
			    ? &importedSkeleton.joints[importedJoint.parentJointIndex].bindModelTransform
			    : nullptr;
			if (!SkeletonTransformContract::IsFinite(importedJoint.inverseBindMatrix)
			    || !SkeletonTransformContract::IsFinite(importedJoint.bindLocalTransform)
			    || !SkeletonTransformContract::IsFinite(importedJoint.parentSpaceTransform)
			    || !SkeletonTransformContract::IsFinite(importedJoint.bindModelTransform)
			    || !SkeletonTransformContract::IsInvertible(importedJoint.inverseBindMatrix)
			    || !SkeletonTransformContract::IsInvertible(importedJoint.bindLocalTransform)
			    || !SkeletonTransformContract::IsTrsDecomposable(importedJoint.bindLocalTransform)
			    || !SkeletonTransformContract::SatisfiesBindInvariant(
			        importedJoint.bindLocalTransform,
			        importedJoint.parentSpaceTransform,
			        parentBindModel,
			        importedJoint.bindModelTransform))
			{
				throw Diagnostics::Error(
				    std::format("Imported skeleton {} joint {} violates the canonical transform contract.", skeletonIndex, jointIndex));
			}

			Assets::CookedSkeletonJointRecord jointRecord;
			CookedSkeletonTranslation::CopyName(importedJoint.name, jointRecord.name);
			jointRecord.sourceNodeIndex = importedJoint.sourceNodeIndex;
			jointRecord.parentJointIndex = importedJoint.parentJointIndex;
			jointRecord.inverseBindMatrix = importedJoint.inverseBindMatrix;
			jointRecord.bindLocalTransform = importedJoint.bindLocalTransform;
			jointRecord.parentSpaceTransform = importedJoint.parentSpaceTransform;
			jointRecord.bindModelTransform = importedJoint.bindModelTransform;
			skeletonAsset.joints.push_back(jointRecord);
		}

		outBuild.manifest.skeletonRefs.push_back(
		    Assets::CookedSceneSkeletonRef{
		        .skeletonAssetId = skeletonAsset.assetId,
		        .sourceSkinIndex = importedSkeleton.sourceSkinIndex,
		        .flags = 0});
		outBuild.outputs.skeletonAssets.push_back(std::move(skeletonAsset));
	}
}
