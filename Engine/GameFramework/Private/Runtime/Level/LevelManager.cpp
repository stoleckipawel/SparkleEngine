#include "PCH.h"
#include "Runtime/Level/LevelManager.h"

#include "Core/Public/Diagnostics/Log.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Level/LevelRegistry.h"
#include "Runtime/Level/LevelChangeEvents.h"
#include "Scene/Scene.h"
#include "Scene/Camera/GameCamera.h"

#include <vector>

LevelManager::LevelManager(Scene& scene) noexcept : m_scene(&scene)
{
	InitializeStartupLevel();
}

void LevelManager::InitializeStartupLevel() noexcept
{
	if (!m_scene)
	{
		LOG_WARNING("LevelManager: Cannot initialize startup level because required services are unavailable");
		return;
	}

	const Level* startupLevel = m_levelRegistry.FindLevelOrDefault(GetStartupLevelName());
	if (!startupLevel)
	{
		LOG_WARNING("LevelManager: Startup level initialization failed because no registered level could be resolved");
		m_activeLevelName.clear();
		return;
	}

	const std::string startupLevelName(startupLevel->GetName());

	const SceneLoadResult loadResult = m_scene->LoadLevel(*startupLevel);
	if (!loadResult.Succeeded())
	{
		m_activeLevelName.clear();
		LOG_WARNING(
		    "LevelManager: Startup level initialization failed for '" + startupLevelName + "'" +
		    (loadResult.errorMessage.empty() ? std::string() : " - " + loadResult.errorMessage));
		return;
	}

	m_activeLevelName = startupLevelName;
	ResetCameraFromLoadedLevel();

	LOG_INFO("LevelManager: Startup level initialized to '" + m_activeLevelName + "'");
}

std::vector<std::string> LevelManager::GetRegisteredLevelNames() const
{
	std::vector<std::string> levelNames;
	levelNames.reserve(m_levelRegistry.GetLevelCount());
	for (const auto& levelEntry : m_levelRegistry.GetAllLevels())
	{
		levelNames.push_back(levelEntry.first);
	}

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

	if (requestedLevelName == m_activeLevelName)
	{
		LOG_DEBUG("LevelManager: Ignoring level change request for the already active level '" + m_activeLevelName + "'");
		return;
	}

	const Level* requestedLevel = m_levelRegistry.FindLevel(requestedLevelName);
	if (!requestedLevel)
	{
		LOG_WARNING("LevelManager: Requested level '" + std::string(requestedLevelName) + "' is not registered");
		return;
	}

	LOG_INFO("LevelManager: Accepted level change request to '" + std::string(requestedLevelName) + "'");
	ProcessLevelChangeRequest(*requestedLevel);
}

SceneLoadResult LevelManager::LoadLevelFromUnloadedState(const Level& level) noexcept
{
	if (!m_scene)
	{
		SceneLoadResult unavailableResult;
		unavailableResult.errorMessage = "Required runtime services are unavailable";
		return unavailableResult;
	}

	LevelWillLoadEventArgs willLoadArgs;
	willLoadArgs.targetLevelName = std::string(level.GetName());
	m_levelChangeEvents.OnLevelWillLoad.Broadcast(willLoadArgs);

	return m_scene->LoadLevel(level);
}

void LevelManager::ResetCameraFromLoadedLevel() noexcept
{
	if (!m_scene)
	{
		return;
	}

	GameCamera& camera = m_scene->GetCamera();
	const LevelCameraDesc& cameraDesc = m_scene->GetCurrentLevelInitialCamera();
	const DirectX::XMFLOAT3 rotationEuler = cameraDesc.transform.GetRotationEuler();

	camera.SetPosition(cameraDesc.transform.GetTranslation());
	camera.SetYawPitch(rotationEuler.y, rotationEuler.x);
}

void LevelManager::ProcessLevelChangeRequest(const Level& requestedLevel) noexcept
{
	if (!m_scene)
	{
		LOG_WARNING("LevelManager: Cannot process level change because required services are unavailable");
		return;
	}

	m_bLevelChangeInProgress = true;

	const std::string previousLevelName = m_activeLevelName;
	const std::string requestedLevelName(requestedLevel.GetName());

	LevelChangeStartedEventArgs startedArgs;
	startedArgs.previousLevelName = previousLevelName;
	startedArgs.requestedLevelName = requestedLevelName;
	m_levelChangeEvents.OnLevelChangeStarted.Broadcast(startedArgs);

	LevelWillUnloadEventArgs willUnloadArgs;
	willUnloadArgs.previousLevelName = previousLevelName;
	willUnloadArgs.requestedLevelName = requestedLevelName;
	m_levelChangeEvents.OnLevelWillUnload.Broadcast(willUnloadArgs);

	m_scene->Clear();
	m_activeLevelName.clear();

	LevelUnloadedEventArgs unloadedArgs;
	unloadedArgs.previousLevelName = previousLevelName;
	m_levelChangeEvents.OnLevelUnloaded.Broadcast(unloadedArgs);

	SceneLoadResult loadResult = LoadLevelFromUnloadedState(requestedLevel);
	if (!loadResult.Succeeded())
	{
		LOG_WARNING(
		    "LevelManager: Level change load failed for '" + requestedLevelName + "'" +
		    (loadResult.errorMessage.empty() ? std::string() : " - " + loadResult.errorMessage));

		LevelLoadFailedEventArgs failedArgs;
		failedArgs.failedLevelName = requestedLevelName;
		failedArgs.fallbackLevelName = std::string(GetEmptyLevelName());
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(failedArgs);

		const Level* fallbackLevel = m_levelRegistry.FindLevel(GetEmptyLevelName());
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

		m_activeLevelName = fallbackLevelName;
	}
	else
	{
		m_activeLevelName = requestedLevelName;
	}

	ResetCameraFromLoadedLevel();

	LevelChangedEventArgs changedArgs;
	changedArgs.previousLevelName = previousLevelName;
	changedArgs.activeLevelName = m_activeLevelName;
	m_levelChangeEvents.OnLevelChanged.Broadcast(changedArgs);

	m_bLevelChangeInProgress = false;

	LOG_INFO("LevelManager: Level change completed from '" + previousLevelName + "' to '" + m_activeLevelName + "'");
}