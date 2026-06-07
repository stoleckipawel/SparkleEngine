#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "SceneSkeleton.h"

#include <cstddef>
#include <vector>

class SPARKLE_ENGINE_API SceneSkeletons final
{
  public:
	void Clear() noexcept;
	void AppendSkeletons(std::vector<SceneSkeletonDesc>&& skeletons);

	std::size_t GetSkeletonCount() const noexcept { return m_skeletons.size(); }
	const std::vector<SceneSkeletonDesc>& GetSkeletons() const noexcept { return m_skeletons; }

	SceneSkeletonPose BuildNeutralPose(Assets::CookedAssetId skeletonAssetId) const;

  private:
	std::vector<SceneSkeletonDesc> m_skeletons;
};
