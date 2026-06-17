#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <cstdint>

class GameScene;

enum class GameSceneUpdatePhase : std::uint8_t
{
	PreAnimation = 0,
	PostAnimation = 1,
};

struct SPARKLE_ENGINE_API GameSceneUpdateContext final
{
	float deltaSeconds = 0.0f;
	GameSceneUpdatePhase phase = GameSceneUpdatePhase::PreAnimation;
};

class SPARKLE_ENGINE_API GameSceneController
{
  public:
	virtual ~GameSceneController();

	virtual void OnSceneReset(GameScene& scene);
	virtual void OnLevelLoaded(GameScene& scene, const LevelDesc& levelDesc);
	virtual void OnSceneAssetsAppended(GameScene& scene);
	virtual void Update(GameScene& scene, const GameSceneUpdateContext& context);
};
