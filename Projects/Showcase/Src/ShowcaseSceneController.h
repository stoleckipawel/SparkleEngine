#pragma once

#include "Scene/GameSceneController.h"
#include "Scene/Transform.h"

#include <cstddef>
#include <vector>

class GameScene;

class ShowcaseSceneController final : public GameSceneController
{
  public:
	void OnSceneReset(GameScene& scene) override;
	void OnSceneAssetsAppended(GameScene& scene) override;
	void Update(GameScene& scene, const GameSceneUpdateContext& context) override;

  private:
	struct AnimatedMesh final
	{
		std::size_t MeshIndex = 0;
		Transform BaseTransform;
		std::size_t LaneIndex = 0;
	};

	void Reset() noexcept;
	void RefreshAnimatedMeshes(GameScene& scene);
	void ApplyMovement(GameScene& scene, float deltaSeconds) noexcept;

	bool m_needsTargetRefresh = true;
	float m_motionTimeSeconds = 0.0f;
	std::vector<AnimatedMesh> m_animatedMeshes;
};
