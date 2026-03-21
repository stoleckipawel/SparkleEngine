#include "PCH.h"
#include "Level/Level.h"

#include "Scene/Camera/GameCameraController.h"
#include "Scene/Lighting/GameSceneLightingState.h"

Level::Level(LevelDesc levelDesc, std::filesystem::path sourcePath) : m_levelDesc(std::move(levelDesc)), m_sourcePath(std::move(sourcePath))
{
}

void Level::ApplyToRuntime(GameCameraController* gameCameraController, GameSceneLightingState* lightingState) const noexcept
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

void Level::CaptureFromRuntime(const GameCameraController* gameCameraController, const GameSceneLightingState* lightingState) noexcept
{
	if (gameCameraController != nullptr)
	{
		m_levelDesc.cameraDesc = gameCameraController->CaptureCurrentCameraDesc();
	}

	if (lightingState != nullptr)
	{
		m_levelDesc.lightingDesc = lightingState->CaptureLevelLightingDesc();
	}
}