#include "PCH.h"
#include "Level/LevelSession.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Environment/EnvironmentVariables.h"
#include "Level/Level.h"
#include "Level/LevelRegistry.h"
#include "Level/Loading/SceneLoadExecutor.h"
#include "World/GameWorld.h"

static const auto g_levelSessionLogger = Logging::GetOrCreateLogger("GameFramework.LevelSession");

class StartupLevelSelection final
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

LevelSession::LevelSession(
    GameWorld& world,
    TaskExecutor& taskExecutor,
    TaskScope& applicationScope) :
    m_gameWorld(&world),
    m_levelRegistry(std::make_unique<LevelRegistry>()),
    m_loadExecutor(std::make_unique<Assets::SceneLoadExecutor>(taskExecutor, applicationScope))
{
	InitializeStartupLevel();
}

LevelSession::~LevelSession() noexcept = default;

void LevelSession::InitializeStartupLevel() noexcept
{
	const std::string requestedName = StartupLevelSelection::ResolveRequestedStartupLevelName();
	const std::string_view startupName = requestedName.empty() ? m_levelRegistry->GetDefaultLevelName() : std::string_view(requestedName);
	LevelAsset* startupLevel = m_levelRegistry->FindLevel(startupName);
	if (!startupLevel)
	{
		SPDLOG_LOGGER_WARN(g_levelSessionLogger, "No registered startup level could be resolved for '{}'.", std::string(startupName));
		return;
	}
	m_pendingLevelChange = startupLevel;
}

std::vector<std::string> LevelSession::GetRegisteredLevelNames() const { return m_levelRegistry->GetLevelNames(); }

void LevelSession::RequestLevelChange(std::string_view requestedLevelName) noexcept
{
	if (requestedLevelName.empty())
		return;
	LevelAsset* requestedLevel = m_levelRegistry->FindLevel(requestedLevelName);
	if (!requestedLevel)
	{
		SPDLOG_LOGGER_WARN(g_levelSessionLogger, "Requested level '{}' is not registered.", std::string(requestedLevelName));
		return;
	}
	if (!m_levelChangeInProgress && m_activeLevel == requestedLevel)
		return;
	if (m_pendingLevelChange == requestedLevel || (m_levelChangeInProgress && m_loadingLevel == requestedLevel))
		return;
	m_pendingLevelChange = requestedLevel;
}

void LevelSession::ProcessPendingLevelChange() noexcept
{
	if (m_pendingLevelChange && m_levelChangeInProgress)
	{
		m_loadExecutor->Cancel();
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

void LevelSession::StartLevelChange(LevelAsset& requestedLevel) noexcept
{
	const std::string previousLevelName = m_activeLevel ? std::string(m_activeLevel->GetName()) : std::string{};
	const std::string requestedLevelName(requestedLevel.GetName());
	const std::uint64_t requestId = m_nextRequestId++;

	m_loadExecutor->Cancel();
	m_latestRequestId = requestId;
	m_loadingLevel = &requestedLevel;
	m_levelChangeInProgress = true;
	m_lastLoadDiagnostic.clear();

	LevelChangeStartedEventArgs startedArgs;
	startedArgs.previousLevelName = previousLevelName;
	startedArgs.requestedLevelName = requestedLevelName;
	m_levelChangeEvents.OnLevelChangeStarted.Broadcast(startedArgs);
	m_levelChangeEvents.OnLevelWillLoad.Broadcast(requestedLevelName);

	try
	{
		m_loadExecutor->Start(
		    requestId,
		    m_gameWorld->GetGeneration(),
		    m_documentGeneration,
		    requestedLevel.BuildDescription());
	}
	catch (const Diagnostics::Error& error)
	{
		m_lastLoadDiagnostic = error.what();
		m_loadingLevel = nullptr;
		m_levelChangeInProgress = false;
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(requestedLevelName);
	}
}

void LevelSession::CompleteLevelChange() noexcept
{
	std::optional<Assets::SceneLoadCompletion> completion = m_loadExecutor->ConsumeSettled();
	if (!completion)
		return;

	if (completion->RequestId != m_latestRequestId)
	{
		Diagnostics::Fatal(g_levelSessionLogger, __FILE__, __LINE__, "Scene-load completion does not match the active request.");
	}

	LevelAsset* loadedLevel = m_loadingLevel;
	m_loadingLevel = nullptr;
	if (!loadedLevel)
	{
		Diagnostics::Fatal(g_levelSessionLogger, __FILE__, __LINE__, "Scene-load completion has no active level.");
	}
	if (completion->Stage != LevelLoadOperationStage::Ready)
	{
		m_lastLoadDiagnostic = std::move(completion->Diagnostic);
		m_levelChangeInProgress = false;
		if (completion->Stage != LevelLoadOperationStage::Cancelled)
			m_levelChangeEvents.OnLevelLoadFailed.Broadcast(loadedLevel->GetName());
		return;
	}

	if (!completion->Package)
	{
		Diagnostics::Fatal(g_levelSessionLogger, __FILE__, __LINE__, "Ready scene-load completion has no package.");
	}
	Assets::SceneLoadPackage& package = *completion->Package;
	const bool stale = package.WorldGeneration != m_gameWorld->GetGeneration() ||
	                   package.DocumentGeneration != m_documentGeneration ||
	                   package.CatalogGeneration != m_loadExecutor->GetCatalogGeneration();
	if (stale)
	{
		m_lastLoadDiagnostic = "Scene load package was rejected because an owner generation changed.";
		m_levelChangeInProgress = false;
		m_levelChangeEvents.OnLevelLoadFailed.Broadcast(loadedLevel->GetName());
		return;
	}

	const std::string previousLevelName = m_activeLevel ? std::string(m_activeLevel->GetName()) : std::string{};
	const std::string loadedLevelName(loadedLevel->GetName());
	m_gameWorld->CommitSceneLoadPackage(std::move(*completion->Package));

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

LevelLoadOperationProgress LevelSession::GetLoadProgress() const noexcept
{
	return m_loadExecutor->GetProgress();
}

bool LevelSession::SaveActiveLevel() noexcept
{
	if (!m_activeLevel || m_levelChangeInProgress)
		return false;
	CaptureSceneToLevel();
	try
	{
		m_levelRegistry->SaveLevel(*m_activeLevel);
		return true;
	}
	catch (const Diagnostics::Error& error)
	{
		m_lastLoadDiagnostic = error.what();
		return false;
	}
}

void LevelSession::CaptureSceneToLevel() noexcept
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
