#include "PCH.h"

#include "Assets/Translators/SceneAssetSkeletonTranslator.h"

#include <utility>

namespace Assets
{
	SkeletonResource BuildSceneAssetSkeleton(
	    const LoadedSkeletonAsset& skeletonAsset,
	    CookedAssetId skeletonAssetId,
	    std::uint32_t sourceSkinIndex)
	{
		SkeletonResource skeleton;
		skeleton.assetId = skeletonAssetId;
		skeleton.sourceSkinIndex = sourceSkinIndex;
		skeleton.joints.reserve(skeletonAsset.joints.size());

		for (const CookedSkeletonJointRecord& jointRecord : skeletonAsset.joints)
		{
			SkeletonJoint joint;
			joint.name = jointRecord.name;
			joint.sourceNodeIndex = jointRecord.sourceNodeIndex;
			joint.parentJointIndex = jointRecord.parentJointIndex;
			joint.inverseBindMatrix = jointRecord.inverseBindMatrix;
			joint.bindLocalTransform = jointRecord.bindLocalTransform;
			joint.parentSpaceTransform = jointRecord.parentSpaceTransform;
			joint.bindModelTransform = jointRecord.bindModelTransform;
			skeleton.joints.push_back(std::move(joint));
		}

		return skeleton;
	}
}
