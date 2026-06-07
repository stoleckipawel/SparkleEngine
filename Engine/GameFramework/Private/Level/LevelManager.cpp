#include "PCH.h"
#include "Level/LevelManager.h"

#include "Assets/SceneAssetManager.h"
#include "Level/Level.h"
#include "Level/LevelRegistry.h"
#include "Level/LevelChangeEvents.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Environment/EnvironmentVariables.h"

#include <algorithm>

static const auto g_levelManagerLogger = Logging::GetOrCreateLogger("GameFramework.LevelManager");

namespace
{
	constexpr std::string_view kDefaultStartupLevelName = "Sponza";
	std::string ResolveStartupLevelName() noexcept
	{
		std::string requestedLevelName;
		if (Environment::TryGetVariable("SPARKLE_STARTUP_LEVEL", requestedLevelName) && !requestedLevelName.empty())
		{
			return requestedLevelName;
		}

		return std::string(kDefaultStartupLevelName);
	}
}

LevelManager::LevelManager(GameScene& scene, Assets::SceneAssetManager& sceneAssetManager) noexcept :
    m_gameScene(&scene), m_sceneAssetManager(&sceneAssetManager)
{
	InitializeStartupLevel();
}

void LevelManager::InitializeStartupLevel() noexcept
{
	if (!m_gameScene)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "LevelManager: Cannot initialize startup level because required services are unavailable");
		return;
	}

	const std::string requestedStartupLevelName = ResolveStartupLevelName();
	LevelAsset* startupLevel = m_levelRegistry.FindLevelOrDefault(requestedStartupLevelName);
	if (!startupLevel)
	{
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Startup level initialization failed because no registered level could be resolved for '{}'",
		    requestedStartupLevelName);
		m_activeLevel = nullptr;
		return;
	}

	const std::string startupLevelName(startupLevel->GetName());

	const GameSceneLoadResult loadResult = LoadLevelFromUnloadedState(*startupLevel);
	if (!loadResult.Succeeded())
	{
		m_activeLevel = nullptr;
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Startup level initialization failed for '{}'{}",
		    startupLevelName,
		    loadResult.errorMessage.empty() ? std::string() : std::string{" - "} + loadResult.errorMessage);
		return;
	}

	m_activeLevel = startupLevel;

	SPDLOG_LOGGER_INFO(g_levelManagerLogger, "LevelManager: Startup level initialized to '{}'", std::string(startupLevel->GetName()));
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
		SPDLOG_LOGGER_DEBUG(
		    g_levelManagerLogger,
		    "LevelManager: Ignoring level change request while another change is already in progress");
		return;
	}

	if (m_activeLevel != nullptr && requestedLevelName == m_activeLevel->GetName())
	{
		SPDLOG_LOGGER_DEBUG(
		    g_levelManagerLogger,
		    "LevelManager: Ignoring level change request for the already active level '{}'",
		    std::string(m_activeLevel->GetName()));
		return;
	}

	LevelAsset* requestedLevel = m_levelRegistry.FindLevel(requestedLevelName);
	if (!requestedLevel)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "LevelManager: Requested level '{}' is not registered", std::string(requestedLevelName));
		return;
	}

	if (m_pendingLevelChange != nullptr)
	{
		if (m_pendingLevelChange == requestedLevel)
		{
			SPDLOG_LOGGER_DEBUG(
			    g_levelManagerLogger,
			    "LevelManager: Ignoring duplicate queued level change request to '{}'",
			    std::string(requestedLevelName));
			return;
		}

		SPDLOG_LOGGER_DEBUG(
		    g_levelManagerLogger,
		    "LevelManager: Replacing queued level change request from '{}' to '{}'",
		    std::string(m_pendingLevelChange->GetName()),
		    std::string(requestedLevelName));
	}

	SPDLOG_LOGGER_INFO(g_levelManagerLogger, "LevelManager: Accepted level change request to '{}'", std::string(requestedLevelName));
	m_pendingLevelChange = requestedLevel;
}

void LevelManager::ProcessPendingLevelChange() noexcept
{
	if (m_bLevelChangeInProgress || m_pendingLevelChange == nullptr)
	{
		return;
	}

	LevelAsset* requestedLevel = m_pendingLevelChange;
	m_pendingLevelChange = nullptr;
	ProcessLevelChangeRequest(*requestedLevel);
}

bool LevelManager::SaveActiveLevel() noexcept
{
	if (m_activeLevel == nullptr)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "LevelManager: Cannot save level state because there is no active level");
		return false;
	}

	CaptureSceneToLevel();

	std::string errorMessage;
	if (!m_levelRegistry.SaveLevel(*m_activeLevel, &errorMessage))
	{
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Failed to persist state for level '{}'{}",
		    std::string(m_activeLevel->GetName()),
		    errorMessage.empty() ? std::string() : std::string{" - "} + errorMessage);
		return false;
	}

	SPDLOG_LOGGER_INFO(
	    g_levelManagerLogger,
	    "LevelManager: Saved all persisted state for level '{}'",
	    std::string(m_activeLevel->GetName()));
	return true;
}

GameSceneLoadResult LevelManager::LoadLevelFromUnloadedState(const LevelAsset& level) noexcept
{
	if (!m_gameScene || !m_sceneAssetManager)
	{
		GameSceneLoadResult unavailableResult;
		unavailableResult.errorMessage = "Required runtime services are unavailable";
		return unavailableResult;
	}

	LevelWillLoadEventArgs willLoadArgs;
	willLoadArgs.targetLevelName = std::string(level.GetName());
	m_levelChangeEvents.OnLevelWillLoad.Broadcast(willLoadArgs);

	m_sceneAssetManager->UnloadAll();

	const LevelDesc& levelDesc = level.GetLevelDesc();
	GameSceneLoadResult loadResult = m_gameScene->LoadLevel(levelDesc);
	if (!loadResult.Succeeded())
	{
		return loadResult;
	}

	Assets::SceneAssetLoadResult sceneAssetLoadResult = m_sceneAssetManager->LoadSceneAssets(levelDesc.sceneAssetIds);
	if (!sceneAssetLoadResult.Succeeded())
	{
		loadResult.status = GameSceneLoadStatus::Failed;
		loadResult.errorMessage = std::move(sceneAssetLoadResult.errorMessage);
		return loadResult;
	}

	const bool hasSceneAssetPayload = sceneAssetLoadResult.sceneAssetPayload.HasMeshes() ||
	                                  !sceneAssetLoadResult.sceneAssetPayload.cameras.empty() ||
	                                  !sceneAssetLoadResult.sceneAssetPayload.lights.empty() ||
	                                  !sceneAssetLoadResult.sceneAssetPayload.skeletons.empty();
	if (hasSceneAssetPayload && !m_gameScene->AppendSceneAssetPayload(std::move(sceneAssetLoadResult.sceneAssetPayload)))
	{
		loadResult.status = GameSceneLoadStatus::Failed;
		loadResult.errorMessage = "GameScene rejected the loaded scene asset payload";
		return loadResult;
	}

	m_gameScene->GetCameras().ApplyPrimaryCamera();

	return loadResult;
}

void LevelManager::CaptureSceneToLevel() noexcept
{
	if (m_activeLevel == nullptr || !m_gameScene)
	{
		return;
	}

	LevelDesc desc = m_activeLevel->BuildDescription();

	desc.lightingDesc = m_gameScene->GetLighting().CaptureToDesc();
	desc.cameraDesc = m_gameScene->GetCameras().GetActiveCamera().CaptureToDesc();

	m_activeLevel->SetLevelDesc(desc);
}

void LevelManager::ProcessLevelChangeRequest(LevelAsset& requestedLevel) noexcept
{
	if (!m_gameScene)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "LevelManager: Cannot process level change because required services are unavailable");
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
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Level change load failed for '{}'{}",
		    requestedLevelName,
		    loadResult.errorMessage.empty() ? std::string() : std::string{" - "} + loadResult.errorMessage);

		LevelLoadFailedEventArgs failedArgs;
		failedArgs.failedLevelName = requestedLevelName;
		failedArgs.fallbackLevelName = std::string(GetEmptyLevelName());
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(failedArgs);

		LevelAsset* fallbackLevel = m_levelRegistry.FindLevel(GetEmptyLevelName());
		if (!fallbackLevel)
		{
			SPDLOG_LOGGER_ERROR(g_levelManagerLogger, "LevelManager: Fallback level 'Empty' is not registered");
			m_bLevelChangeInProgress = false;
			return;
		}

		const std::string fallbackLevelName(fallbackLevel->GetName());

		loadResult = LoadLevelFromUnloadedState(*fallbackLevel);
		if (!loadResult.Succeeded())
		{
			SPDLOG_LOGGER_ERROR(
			    g_levelManagerLogger,
			    "LevelManager: Fallback level '{}' failed to load{}",
			    fallbackLevelName,
			    loadResult.errorMessage.empty() ? std::string() : std::string{" - "} + loadResult.errorMessage);
			m_bLevelChangeInProgress = false;
			return;
		}

		m_activeLevel = fallbackLevel;
	}
	else
	{
		m_activeLevel = &requestedLevel;
	}

	LevelChangedEventArgs changedArgs;
	changedArgs.previousLevelName = previousLevelName;
	changedArgs.activeLevelName = m_activeLevel != nullptr ? std::string(m_activeLevel->GetName()) : std::string();
	m_levelChangeEvents.OnLevelChanged.Broadcast(changedArgs);

	m_bLevelChangeInProgress = false;

	SPDLOG_LOGGER_INFO(
	    g_levelManagerLogger,
	    "LevelManager: Level change completed from '{}' to '{}'",
	    previousLevelName,
	    m_activeLevel != nullptr ? std::string(m_activeLevel->GetName()) : std::string());
}
