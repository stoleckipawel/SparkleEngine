#pragma once

#include "Scene/Transform.h"

#include <cstddef>
#include <vector>

class GameScene;

class ShowcaseSceneBehavior final
{
  public:
	void Update(GameScene& scene, float deltaSeconds) noexcept;

  private:
	struct AnimatedMesh final
	{
		std::size_t MeshIndex = 0;
		Transform BaseTransform;
	};

	void Reset() noexcept;
	void CaptureSponzaAnimatedMeshes(GameScene& scene);
	void UpdateSponzaPtlasPatrol(GameScene& scene, float deltaSeconds) noexcept;

	float m_sponzaPatrolTimeSeconds = 0.0f;
	std::size_t m_lastSponzaMeshCount = 0;
	std::vector<AnimatedMesh> m_sponzaAnimatedMeshes;
};
