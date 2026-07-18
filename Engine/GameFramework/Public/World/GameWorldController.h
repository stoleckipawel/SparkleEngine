#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <cstdint>

class GameWorld;

enum class GameWorldUpdatePhase : std::uint8_t
{
	PreAnimation = 0,
	PostAnimation = 1,
};

struct SPARKLE_ENGINE_API GameWorldUpdateContext final
{
	float deltaSeconds = 0.0f;
	GameWorldUpdatePhase phase = GameWorldUpdatePhase::PreAnimation;
};

class SPARKLE_ENGINE_API GameWorldController
{
  public:
	virtual ~GameWorldController();

	virtual void OnWorldReset(GameWorld& world);
	virtual void OnLevelLoaded(GameWorld& world, const LevelDesc& levelDesc);
	virtual void OnSceneAssetsAppended(GameWorld& world);
	virtual void Update(GameWorld& world, const GameWorldUpdateContext& context);
};
