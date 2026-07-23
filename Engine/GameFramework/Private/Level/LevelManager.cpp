#include "PCH.h"
#include "Level/LevelManager.h"

#include "Environment/EnvironmentVariables.h"
#include "Level/Level.h"
#include "Level/LevelRegistry.h"
#include "Level/Loading/SceneLoadExecutionService.h"
#include "World/GameWorld.h"

static const auto g_levelManagerLogger = Logging::GetOrCreateLogger("GameFramework.LevelManager");

class LevelManagerOperations final
{
  public:
	static std::string ResolveRequestedStartupLevelName() noexcept
	{
		std::string requestedLevelName;
		return Environment::TryGetVariable("SPARKLE_STARTUP_LEVEL", requestedLevelName) && !requestedLevelName.empty()
		           ? requestedLevelName
		           : std::string{};
	}

};

LevelManager::LevelManager(
    GameWorld& world,
    TaskExecutor& taskExecutor,
    TaskScope& applicationScope) :
    m_gameWorld(&world),
    m_levelRegistry(std::make_unique<LevelRegistry>()),
    m_loadExecution(std::make_unique<Assets::SceneLoadExecutionService>(taskExecutor, applicationScope))
{
	InitializeStartupLevel();
}

LevelManager::~LevelManager() noexcept = default;

void LevelManager::InitializeStartupLevel() noexcept
{
	const std::string requestedName = LevelManagerOperations::ResolveRequestedStartupLevelName();
	const std::string_view startupName = requestedName.empty() ? m_levelRegistry->GetDefaultLevelName() : std::string_view(requestedName);
	LevelAsset* startupLevel = m_levelRegistry->FindLevel(startupName);
	if (!startupLevel)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "No registered startup level could be resolved for '{}'.", std::string(startupName));
		return;
	}
	m_pendingLevelChange = startupLevel;
}

std::vector<std::string> LevelManager::GetRegisteredLevelNames() const { return m_levelRegistry->GetLevelNames(); }

void LevelManager::RequestLevelChange(std::string_view requestedLevelName) noexcept
{
	if (requestedLevelName.empty())
		return;
	LevelAsset* requestedLevel = m_levelRegistry->FindLevel(requestedLevelName);
	if (!requestedLevel)
	{
		SPDLOG_LOGGER_WARN(g_levelManagerLogger, "Requested level '{}' is not registered.", std::string(requestedLevelName));
		return;
	}
	if (!m_levelChangeInProgress && m_activeLevel == requestedLevel)
		return;
	if (m_pendingLevelChange == requestedLevel || (m_levelChangeInProgress && m_loadingLevel == requestedLevel))
		return;
	m_pendingLevelChange = requestedLevel;
}

void LevelManager::ProcessPendingLevelChange() noexcept
{
	if (m_pendingLevelChange && m_levelChangeInProgress)
	{
		m_loadExecution->Cancel();
		CompleteLevelChange();
		if (m_levelChangeInProgress)
			return;
	}
	if (m_pendingLevelChange)
	{
		LevelAsset* requestedLevel = m_pendingLevelChange;
		m_pendingLevelChange = nullptr;
		StartLevelChange(*requestedLevel);
	}
	CompleteLevelChange();
}

void LevelManager::StartLevelChange(LevelAsset& requestedLevel) noexcept
{
	const std::string previousLevelName = m_activeLevel ? std::string(m_activeLevel->GetName()) : std::string{};
	const std::string requestedLevelName(requestedLevel.GetName());
	const std::uint64_t requestId = m_nextRequestId++;

	m_loadExecution->Cancel();
	m_latestRequestId = requestId;
	m_loadingLevel = &requestedLevel;
	m_levelChangeInProgress = true;
	m_lastLoadDiagnostic.clear();

	LevelChangeStartedEventArgs startedArgs;
	startedArgs.previousLevelName = previousLevelName;
	startedArgs.requestedLevelName = requestedLevelName;
	m_levelChangeEvents.OnLevelChangeStarted.Broadcast(startedArgs);
	m_levelChangeEvents.OnLevelWillLoad.Broadcast(requestedLevelName);

	std::string errorMessage;
	if (!m_loadExecution->Start(
	        requestId,
	        m_gameWorld->GetGeneration(),
	        m_documentGeneration,
	        requestedLevel.BuildDescription(),
	        errorMessage))
	{
		m_lastLoadDiagnostic = std::move(errorMessage);
		m_loadingLevel = nullptr;
		m_levelChangeInProgress = false;
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(requestedLevelName);
	}
}

void LevelManager::CompleteLevelChange() noexcept
{
	Assets::SceneLoadCompletion completion;
	if (!m_loadExecution->TryConsume(completion))
		return;

	if (completion.RequestId != m_latestRequestId)
		return;

	LevelAsset* loadedLevel = m_loadingLevel;
	m_loadingLevel = nullptr;
	if (!completion.Succeeded() || !loadedLevel)
	{
		m_lastLoadDiagnostic = std::move(completion.Diagnostic);
		m_levelChangeInProgress = false;
		if (completion.Stage != LevelLoadOperationStage::Cancelled && loadedLevel)
			m_levelChangeEvents.OnLevelLoadFailed.Broadcast(loadedLevel->GetName());
		return;
	}

	Assets::SceneLoadPackage& package = *completion.Package;
	const bool stale = package.WorldGeneration != m_gameWorld->GetGeneration() ||
	                   package.DocumentGeneration != m_documentGeneration ||
	                   package.CatalogGeneration != m_loadExecution->GetCatalogGeneration();
	if (stale)
	{
		m_lastLoadDiagnostic = "Scene load package was rejected because an owner generation changed.";
		m_levelChangeInProgress = false;
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(loadedLevel->GetName());
		return;
	}

	const std::string previousLevelName = m_activeLevel ? std::string(m_activeLevel->GetName()) : std::string{};
	const std::string loadedLevelName(loadedLevel->GetName());
	std::string commitError;
	if (!m_gameWorld->CommitSceneLoadPackage(std::move(*completion.Package), commitError))
	{
		m_lastLoadDiagnostic = std::move(commitError);
		m_levelChangeInProgress = false;
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(loadedLevelName);
		return;
	}

	LevelWillUnloadEventArgs willUnloadArgs;
	willUnloadArgs.previousLevelName = previousLevelName;
	willUnloadArgs.requestedLevelName = loadedLevelName;
	m_levelChangeEvents.OnLevelWillUnload.Broadcast(willUnloadArgs);
	if (!previousLevelName.empty())
		m_levelChangeEvents.OnLevelUnloaded.Broadcast(previousLevelName);
	m_gameWorld->FinalizeSceneLoadCommit();
	m_activeLevel = loadedLevel;
	LevelChangedEventArgs changedArgs;
	changedArgs.previousLevelName = previousLevelName;
	changedArgs.activeLevelName = loadedLevelName;
	m_levelChangeEvents.OnLevelChanged.Broadcast(changedArgs);
	m_levelChangeInProgress = false;
	m_lastLoadDiagnostic.clear();
}

LevelLoadOperationProgress LevelManager::GetLoadProgress() const noexcept
{
	return m_loadExecution->GetProgress();
}

bool LevelManager::SaveActiveLevel() noexcept
{
	if (!m_activeLevel || m_levelChangeInProgress)
		return false;
	CaptureSceneToLevel();
	std::string errorMessage;
	if (m_levelRegistry->SaveLevel(*m_activeLevel, &errorMessage))
		return true;
	m_lastLoadDiagnostic = std::move(errorMessage);
	return false;
}

void LevelManager::CaptureSceneToLevel() noexcept
{
	if (!m_activeLevel || !m_gameWorld)
		return;
	LevelDesc desc = m_activeLevel->BuildDescription();
	const WorldReadView view = m_gameWorld->AcquireReadView();
	desc.lights.clear();
	desc.lights.reserve(view.GetLights().size());
	for (const WorldLightReadData& light : view.GetLights()) desc.lights.push_back(light.Description);
	desc.sky = view.GetSkyEnvironment() ? std::optional<SceneSkyDesc>(view.GetSkyEnvironment()->Description) : std::nullopt;
	for (const WorldCameraReadData& camera : view.GetCameras())
	{
		if (camera.Active)
		{
			desc.cameraDesc = camera.Description;
			break;
		}
	}
	m_activeLevel->SetLevelDesc(desc);
}
