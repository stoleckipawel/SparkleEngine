#include "PCH.h"
#include "Runtime/Level/LevelManager.h"

#include "Core/Public/Diagnostics/Log.h"
#include "Level/Level.h"
#include "Level/LevelRegistry.h"
#include "Runtime/Level/LevelChangeEvents.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/GameCameraController.h"
#include "Scene/Lighting/GameSceneLightingState.h"

#include <algorithm>

LevelManager::LevelManager(GameScene& scene) noexcept : m_gameScene(&scene)
{
	InitializeStartupLevel();
}

void LevelManager::InitializeStartupLevel() noexcept
{
	if (!m_gameScene)
	{
		LOG_WARNING("LevelManager: Cannot initialize startup level because required services are unavailable");
		return;
	}

	Level* startupLevel = m_levelRegistry.FindLevelOrDefault(GetStartupLevelName());
	if (!startupLevel)
	{
		LOG_WARNING("LevelManager: Startup level initialization failed because no registered level could be resolved");
		m_activeLevel = nullptr;
		return;
	}

	const std::string startupLevelName(startupLevel->GetName());

	const GameSceneLoadResult loadResult = m_gameScene->LoadLevel(*startupLevel);
	if (!loadResult.Succeeded())
	{
		m_activeLevel = nullptr;
		LOG_WARNING(
		    "LevelManager: Startup level initialization failed for '" + startupLevelName + "'" +
		    (loadResult.errorMessage.empty() ? std::string() : " - " + loadResult.errorMessage));
		return;
	}

	m_activeLevel = startupLevel;
	InitializeActiveLevel();

	LOG_INFO("LevelManager: Startup level initialized to '" + std::string(startupLevel->GetName()) + "'");
}

std::vector<std::string> LevelManager::GetRegisteredLevelNames() const
{
	std::vector<std::string> levelNames;
	levelNames.reserve(m_levelRegistry.GetLevelCount());
	for (const auto& levelEntry : m_levelRegistry.GetAllLevels())
	{
		levelNames.push_back(levelEntry.first);
	}

	std::sort(levelNames.begin(), levelNames.end());

	return levelNames;
}

void LevelManager::RequestLevelChange(std::string_view requestedLevelName) noexcept
{
	if (requestedLevelName.empty())
	{
		return;
	}

	if (m_bLevelChangeInProgress)
	{
		LOG_DEBUG("LevelManager: Ignoring level change request while another change is already in progress");
		return;
	}

	if (m_activeLevel != nullptr && requestedLevelName == m_activeLevel->GetName())
	{
		LOG_DEBUG(
		    "LevelManager: Ignoring level change request for the already active level '" + std::string(m_activeLevel->GetName()) + "'");
		return;
	}

	Level* requestedLevel = m_levelRegistry.FindLevel(requestedLevelName);
	if (!requestedLevel)
	{
		LOG_WARNING("LevelManager: Requested level '" + std::string(requestedLevelName) + "' is not registered");
		return;
	}

	LOG_INFO("LevelManager: Accepted level change request to '" + std::string(requestedLevelName) + "'");
	ProcessLevelChangeRequest(*requestedLevel);
}

void LevelManager::RegisterGameCameraController(GameCameraController& gameCameraController) noexcept
{
	m_gameCameraController = &gameCameraController;
	InitializeActiveLevel();
}

GameSceneLightingState* LevelManager::GetGameSceneLightingState() noexcept
{
	return m_gameScene != nullptr ? &m_gameScene->GetLightingState() : nullptr;
}

const GameSceneLightingState* LevelManager::GetGameSceneLightingState() const noexcept
{
	return m_gameScene != nullptr ? &m_gameScene->GetLightingState() : nullptr;
}

bool LevelManager::SaveActiveLevel() noexcept
{
	if (m_activeLevel == nullptr)
	{
		LOG_WARNING("LevelManager: Cannot save level state because there is no active level");
		return false;
	}

	m_activeLevel->CaptureFromRuntime(m_gameCameraController, GetGameSceneLightingState());

	std::string errorMessage;
	if (!m_levelRegistry.SaveLevel(*m_activeLevel, &errorMessage))
	{
		LOG_WARNING(
		    "LevelManager: Failed to persist state for level '" + std::string(m_activeLevel->GetName()) + "'" +
		    (errorMessage.empty() ? std::string() : " - " + errorMessage));
		return false;
	}

	LOG_INFO("LevelManager: Saved all persisted state for level '" + std::string(m_activeLevel->GetName()) + "'");
	return true;
}

GameSceneLoadResult LevelManager::LoadLevelFromUnloadedState(const Level& level) noexcept
{
	if (!m_gameScene)
	{
		GameSceneLoadResult unavailableResult;
		unavailableResult.errorMessage = "Required runtime services are unavailable";
		return unavailableResult;
	}

	LevelWillLoadEventArgs willLoadArgs;
	willLoadArgs.targetLevelName = std::string(level.GetName());
	m_levelChangeEvents.OnLevelWillLoad.Broadcast(willLoadArgs);

	return m_gameScene->LoadLevel(level);
}

void LevelManager::InitializeActiveLevel() noexcept
{
	if (m_activeLevel == nullptr)
	{
		LOG_DEBUG("LevelManager: Skipping level initialization because there is no active level");
		return;
	}

	if (m_gameCameraController == nullptr && GetGameSceneLightingState() == nullptr)
	{
		LOG_DEBUG("LevelManager: Skipping level initialization because runtime state is unavailable");
		return;
	}

	m_activeLevel->ApplyToRuntime(m_gameCameraController, GetGameSceneLightingState());
}

void LevelManager::ProcessLevelChangeRequest(Level& requestedLevel) noexcept
{
	if (!m_gameScene)
	{
		LOG_WARNING("LevelManager: Cannot process level change because required services are unavailable");
		return;
	}

	m_bLevelChangeInProgress = true;

	const std::string previousLevelName = m_activeLevel != nullptr ? std::string(m_activeLevel->GetName()) : std::string();
	const std::string requestedLevelName(requestedLevel.GetName());

	LevelChangeStartedEventArgs startedArgs;
	startedArgs.previousLevelName = previousLevelName;
	startedArgs.requestedLevelName = requestedLevelName;
	m_levelChangeEvents.OnLevelChangeStarted.Broadcast(startedArgs);

	LevelWillUnloadEventArgs willUnloadArgs;
	willUnloadArgs.previousLevelName = previousLevelName;
	willUnloadArgs.requestedLevelName = requestedLevelName;
	m_levelChangeEvents.OnLevelWillUnload.Broadcast(willUnloadArgs);

	m_gameScene->Clear();
	m_activeLevel = nullptr;

	LevelUnloadedEventArgs unloadedArgs;
	unloadedArgs.previousLevelName = previousLevelName;
	m_levelChangeEvents.OnLevelUnloaded.Broadcast(unloadedArgs);

	GameSceneLoadResult loadResult = LoadLevelFromUnloadedState(requestedLevel);
	if (!loadResult.Succeeded())
	{
		LOG_WARNING(
		    "LevelManager: Level change load failed for '" + requestedLevelName + "'" +
		    (loadResult.errorMessage.empty() ? std::string() : " - " + loadResult.errorMessage));

		LevelLoadFailedEventArgs failedArgs;
		failedArgs.failedLevelName = requestedLevelName;
		failedArgs.fallbackLevelName = std::string(GetEmptyLevelName());
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(failedArgs);

		Level* fallbackLevel = m_levelRegistry.FindLevel(GetEmptyLevelName());
		if (!fallbackLevel)
		{
			LOG_ERROR("LevelManager: Fallback level 'Empty' is not registered");
			m_bLevelChangeInProgress = false;
			return;
		}

		const std::string fallbackLevelName(fallbackLevel->GetName());

		loadResult = LoadLevelFromUnloadedState(*fallbackLevel);
		if (!loadResult.Succeeded())
		{
			LOG_ERROR(
			    "LevelManager: Fallback level '" + fallbackLevelName + "' failed to load" +
			    (loadResult.errorMessage.empty() ? std::string() : " - " + loadResult.errorMessage));
			m_bLevelChangeInProgress = false;
			return;
		}

		m_activeLevel = fallbackLevel;
	}
	else
	{
		m_activeLevel = &requestedLevel;
	}

	InitializeActiveLevel();

	LevelChangedEventArgs changedArgs;
	changedArgs.previousLevelName = previousLevelName;
	changedArgs.activeLevelName = m_activeLevel != nullptr ? std::string(m_activeLevel->GetName()) : std::string();
	m_levelChangeEvents.OnLevelChanged.Broadcast(changedArgs);

	m_bLevelChangeInProgress = false;

	LOG_INFO(
	    "LevelManager: Level change completed from '" + previousLevelName + "' to '" +
	    (m_activeLevel != nullptr ? std::string(m_activeLevel->GetName()) : std::string()) + "'");
}