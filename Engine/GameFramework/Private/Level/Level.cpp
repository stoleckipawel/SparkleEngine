#include "PCH.h"
#include "Level/Level.h"

#include "Scene/Camera/GameCameraController.h"
#include "Scene/Lighting/GameSceneLightingState.h"

Level::Level(LevelDesc levelDesc, std::filesystem::path sourcePath) : m_levelDesc(std::move(levelDesc)), m_sourcePath(std::move(sourcePath))
{
}

void Level::Initialize(GameCameraController* gameCameraController, GameSceneLightingState* lightingState) const noexcept
{
	if (gameCameraController != nullptr)
	{
		gameCameraController->ApplyCameraDesc(m_levelDesc.cameraDesc);
	}

	if (lightingState != nullptr)
	{
		lightingState->ApplyLevelLightingDesc(m_levelDesc.lightingDesc);
	}
}