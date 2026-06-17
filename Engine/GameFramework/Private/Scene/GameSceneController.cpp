#include "PCH.h"
#include "Scene/GameSceneController.h"

GameSceneController::~GameSceneController() = default;

void GameSceneController::OnSceneReset(GameScene& scene)
{
}

void GameSceneController::OnLevelLoaded(GameScene& scene, const LevelDesc& levelDesc)
{
}

void GameSceneController::OnSceneAssetsAppended(GameScene& scene)
{
}

void GameSceneController::Update(GameScene& scene, const GameSceneUpdateContext& context)
{
}
