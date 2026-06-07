#include "PCH.h"

#include "Scene/Skeletons/SceneSkeletons.h"

#include <utility>

void SceneSkeletons::Clear() noexcept
{
	m_skeletons.clear();
}

void SceneSkeletons::AppendSkeletons(std::vector<SceneSkeletonDesc>&& skeletons)
{
	m_skeletons.reserve(m_skeletons.size() + skeletons.size());
	for (SceneSkeletonDesc& skeleton : skeletons)
	{
		m_skeletons.push_back(std::move(skeleton));
	}
}

SceneSkeletonPose SceneSkeletons::BuildNeutralPose(Assets::CookedAssetId skeletonAssetId) const
{
	SceneSkeletonPose pose;
	pose.skeletonAssetId = skeletonAssetId;
	for (const SceneSkeletonDesc& skeleton : m_skeletons)
	{
		if (skeleton.assetId != skeletonAssetId)
		{
			continue;
		}

		pose.jointMatrices.reserve(skeleton.joints.size());
		for (const SceneJointDesc& joint : skeleton.joints)
		{
			pose.jointMatrices.push_back(joint.bindPoseWorldTransform);
		}
		break;
	}

	return pose;
}
