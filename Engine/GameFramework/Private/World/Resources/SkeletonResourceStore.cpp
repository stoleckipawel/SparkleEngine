#include "PCH.h"

#include "World/Resources/SkeletonResourceStore.h"

void SkeletonResourceStore::Append(std::vector<SceneSkeletonDesc>&& skeletons)
{
	m_skeletons.reserve(m_skeletons.size() + skeletons.size());
	for (SceneSkeletonDesc& skeleton : skeletons)
		m_skeletons.push_back(std::move(skeleton));
}

SceneSkeletonPose SkeletonResourceStore::BuildNeutralPose(Assets::CookedAssetId skeletonAssetId) const
{
	SceneSkeletonPose pose;
	pose.skeletonAssetId = skeletonAssetId;
	for (const SceneSkeletonDesc& skeleton : m_skeletons)
	{
		if (skeleton.assetId != skeletonAssetId)
			continue;
		for (const SceneJointDesc& joint : skeleton.joints)
			pose.jointMatrices.push_back(joint.bindPoseWorldTransform);
		break;
	}
	return pose;
}
