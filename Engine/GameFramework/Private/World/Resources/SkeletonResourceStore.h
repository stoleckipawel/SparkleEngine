#pragma once

#include "GameFramework/Public/Scene/Skeletons/SceneSkeleton.h"

#include <vector>

class SkeletonResourceStore final
{
  public:
	void Append(std::vector<SceneSkeletonDesc>&& skeletons);
	const std::vector<SceneSkeletonDesc>& GetAll() const noexcept { return m_skeletons; }
	SceneSkeletonPose BuildNeutralPose(Assets::CookedAssetId skeletonAssetId) const;

  private:
	std::vector<SceneSkeletonDesc> m_skeletons;
};
