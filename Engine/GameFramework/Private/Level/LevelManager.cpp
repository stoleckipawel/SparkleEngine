#include "PCH.h"
#include "Level/LevelManager.h"

#include "Assets/SceneAssetManager.h"
#include "Level/Level.h"
#include "Level/LevelRegistry.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Sky/SceneSky.h"
#include "Environment/EnvironmentVariables.h"

#include <memory>

static const auto g_levelManagerLogger = Logging::GetOrCreateLogger("GameFramework.LevelManager");

namespace
{
	std::string ResolveRequestedStartupLevelName() noexcept
	{
		std::string requestedLevelName;
		if (Environment::TryGetVariable("SPARKLE_STARTUP_LEVEL", requestedLevelName) && !requestedLevelName.empty())
		{
			return requestedLevelName;
		}

		return {};
	}
}

LevelManager::LevelManager(GameScene& scene, Assets::SceneAssetManager& sceneAssetManager) :
    m_gameScene(&scene), m_sceneAssetManager(&sceneAssetManager), m_levelRegistry(std::make_unique<LevelRegistry>())
{
	InitializeStartupLevel();
}

LevelManager::~LevelManager() noexcept = default;

void LevelManager::InitializeStartupLevel() noexcept
{
	if (!m_gameScene)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "LevelManager: Cannot initialize startup level because required services are unavailable");
		return;
	}

	const std::string requestedStartupLevelName = ResolveRequestedStartupLevelName();
	const std::string_view defaultLevelName = m_levelRegistry->GetDefaultLevelName();
	LevelAsset* startupLevel =
	    m_levelRegistry->FindLevel(requestedStartupLevelName.empty() ? defaultLevelName : std::string_view(requestedStartupLevelName));
	if (!startupLevel)
	{
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Startup level initialization failed because no registered level could be resolved for '{}'",
		    requestedStartupLevelName.empty() ? std::string(m_levelRegistry->GetDefaultLevelName()) : requestedStartupLevelName);
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
}

std::vector<std::string> LevelManager::GetRegisteredLevelNames() const
{
	return m_levelRegistry->GetLevelNames();
}

void LevelManager::RequestLevelChange(std::string_view requestedLevelName) noexcept
{
	if (requestedLevelName.empty())
	{
		return;
	}

	if (m_bLevelChangeInProgress)
	{
		return;
	}

	if (m_activeLevel != nullptr && requestedLevelName == m_activeLevel->GetName())
	{
		return;
	}

	LevelAsset* requestedLevel = m_levelRegistry->FindLevel(requestedLevelName);
	if (!requestedLevel)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "LevelManager: Requested level '{}' is not registered", std::string(requestedLevelName));
		return;
	}

	if (m_pendingLevelChange != nullptr)
	{
		if (m_pendingLevelChange == requestedLevel)
		{
			return;
		}
	}

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
	if (!m_levelRegistry->SaveLevel(*m_activeLevel, &errorMessage))
	{
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Failed to persist state for level '{}'{}",
		    std::string(m_activeLevel->GetName()),
		    errorMessage.empty() ? std::string() : std::string{" - "} + errorMessage);
		return false;
	}

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

	m_levelChangeEvents.OnLevelWillLoad.Broadcast(level.GetName());

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

	const bool hasSceneAssetPayload =
	    sceneAssetLoadResult.sceneAssetPayload.HasMeshes() || !sceneAssetLoadResult.sceneAssetPayload.cameras.empty() ||
	    !sceneAssetLoadResult.sceneAssetPayload.lights.empty() || !sceneAssetLoadResult.sceneAssetPayload.skeletons.empty();
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

	desc.lights = m_gameScene->GetLighting().CaptureToDesc();
	desc.sky = m_gameScene->GetSky().CaptureToDesc();
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

	m_levelChangeEvents.OnLevelUnloaded.Broadcast(previousLevelName);

	GameSceneLoadResult loadResult = LoadLevelFromUnloadedState(requestedLevel);
	if (!loadResult.Succeeded())
	{
		SPDLOG_LOGGER_WARN(
		    g_levelManagerLogger,
		    "LevelManager: Level change load failed for '{}'{}",
		    requestedLevelName,
		    loadResult.errorMessage.empty() ? std::string() : std::string{" - "} + loadResult.errorMessage);

		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(requestedLevelName);
		m_bLevelChangeInProgress = false;
		return;
	}

	m_activeLevel = &requestedLevel;

	LevelChangedEventArgs changedArgs;
	changedArgs.previousLevelName = previousLevelName;
	changedArgs.activeLevelName = m_activeLevel != nullptr ? std::string(m_activeLevel->GetName()) : std::string();
	m_levelChangeEvents.OnLevelChanged.Broadcast(changedArgs);

	m_bLevelChangeInProgress = false;
}
