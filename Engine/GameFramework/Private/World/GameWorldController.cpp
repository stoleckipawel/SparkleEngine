#include "PCH.h"
#include "World/GameWorldController.h"

GameWorldController::~GameWorldController() = default;

void GameWorldController::OnWorldReset(GameWorld& world)
{
}

void GameWorldController::OnLevelLoaded(GameWorld& world, const LevelDesc& levelDesc)
{
}

void GameWorldController::OnSceneAssetsAppended(GameWorld& world)
{
}

void GameWorldController::Update(GameWorld& world, const GameWorldUpdateContext& context)
{
}
