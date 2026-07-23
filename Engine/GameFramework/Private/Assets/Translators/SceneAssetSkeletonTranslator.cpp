#include "PCH.h"

#include "Assets/Translators/SceneAssetSkeletonTranslator.h"

#include <string>
#include <utility>

namespace Assets
{

		std::string JointNameToString(const CookedSkeletonJointRecord& jointRecord)
		{
			std::size_t length = 0;
			while (length < sizeof(jointRecord.name) && jointRecord.name[length] != '\0')
			{
				++length;
			}

			return std::string(jointRecord.name, length);
		}


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
			joint.name = JointNameToString(jointRecord);
			joint.sourceNodeIndex = jointRecord.sourceNodeIndex;
			joint.parentJointIndex = jointRecord.parentJointIndex;
			joint.inverseBindMatrix = jointRecord.inverseBindMatrix;
			joint.bindPoseWorldTransform = jointRecord.bindPoseWorldTransform;
			skeleton.joints.push_back(std::move(joint));
		}

		return skeleton;
	}
}
