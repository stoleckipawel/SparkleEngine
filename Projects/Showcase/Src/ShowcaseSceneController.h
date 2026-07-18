#pragma once

#include "World/GameWorldController.h"
#include "Scene/Transform.h"
#include "World/EntityId.h"

#include <cstddef>
#include <vector>

class GameWorld;

class ShowcaseSceneController final : public GameWorldController
{
  public:
	void OnWorldReset(GameWorld& world) override;
	void OnSceneAssetsAppended(GameWorld& world) override;
	void Update(GameWorld& world, const GameWorldUpdateContext& context) override;

  private:
	struct AnimatedMesh final
	{
		EntityId MeshEntity;
		Transform BaseTransform;
		std::size_t LaneIndex = 0;
	};

	void Reset() noexcept;
	void RefreshAnimatedMeshes(GameWorld& world);
	void ApplyMovement(GameWorld& world, float deltaSeconds) noexcept;

	bool m_needsTargetRefresh = true;
	float m_motionTimeSeconds = 0.0f;
	std::vector<AnimatedMesh> m_animatedMeshes;
};
