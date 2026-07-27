#include "PCH.h"

#include "Features/Skeletons/CookedSceneSkeletonBuilder.h"

#include "Core/Public/Hash/HashUtils.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>

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
		const std::size_t copyLength = (std::min)(sourceName.size(), sizeof(destination) - 1u);
		std::memcpy(destination, sourceName.data(), copyLength);
	}
};

void CookedSceneSkeletonBuilder::BuildSkeletons(
	const SourceImportResult& importResult,
	std::string_view sceneAssetId,
	CookedSceneBuild& outBuild)
{
	outBuild.outputs.skeletonAssets.clear();
	outBuild.manifest.skeletonRefs.clear();
	outBuild.outputs.skeletonAssets.reserve(importResult.scene.skeletons.size());
	outBuild.manifest.skeletonRefs.reserve(importResult.scene.skeletons.size());

	for (const ImportedSkeleton& importedSkeleton : importResult.scene.skeletons)
	{
		if (!importedSkeleton.IsValid())
		{
			continue;
		}

		CookedSkeletonAssetBuild skeletonAsset;
		skeletonAsset.assetId = CookedSkeletonTranslation::BuildSkeletonAssetId(sceneAssetId, importedSkeleton.sourceSkinIndex);
		skeletonAsset.sourceSkinIndex = importedSkeleton.sourceSkinIndex;
		skeletonAsset.sourcePath = importResult.scene.sourcePath;
		skeletonAsset.joints.reserve(importedSkeleton.joints.size());

		for (const ImportedJoint& importedJoint : importedSkeleton.joints)
		{
			Assets::CookedSkeletonJointRecord jointRecord;
			CookedSkeletonTranslation::CopyName(importedJoint.name, jointRecord.name);
			jointRecord.sourceNodeIndex = importedJoint.sourceNodeIndex;
			jointRecord.parentJointIndex = importedJoint.parentJointIndex;
			jointRecord.inverseBindMatrix = importedJoint.inverseBindMatrix;
			jointRecord.bindPoseWorldTransform = importedJoint.bindPoseWorldTransform;
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
