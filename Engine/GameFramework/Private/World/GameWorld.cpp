#include "PCH.h"
#include "World/GameWorld.h"

#include "Assets/SceneAssetPayload.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "GameFramework/Public/Scene/Camera/CameraNavigation.h"
#include "World/GameWorldSceneAssetCommitter.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "World/GameWorldState.h"
#include "World/Extraction/RenderFrameSubmissionExtractor.h"
#include "World/Editing/WorldEditCommandQueue.h"
#include "Level/Loading/SceneLoadPackage.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/Systems/GameWorldSystems.h"

#include <utility>

static const auto g_gameWorldLogger = Logging::GetOrCreateLogger("GameFramework.GameWorld");

GameWorld::GameWorld(TaskExecutor& taskExecutor) :
    m_state(std::make_unique<ECS::GameWorldState>()),
    m_resources(std::make_unique<GameWorldResourceStores>()),
    m_taskExecutor(taskExecutor),
    m_editCommands(std::make_unique<WorldEditCommandQueue>()),
    m_renderFrameSubmissionExtractor(std::make_unique<ECS::RenderFrameSubmissionExtractor>())
{
}

GameWorld::~GameWorld() noexcept = default;

void GameWorld::InitializeStagedLevel(const LevelDesc& desc)
{
	m_activeLevelName = desc.name;
	m_activeLevelDesc = desc;
	if (m_state->GetCameraCount() == 0)
	{
		SceneCameraEntry entry;
		entry.name = "Scene Camera";
		entry.desc = desc.cameraDesc;
		if (!m_state->AddCamera(std::move(entry), true).IsValid())
		{
			Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "The staged world rejected its level camera.");
		}
	}
	else
	{
		if (!m_state->WriteCameraDesc(m_state->GetActiveCamera(), desc.cameraDesc))
		{
			Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "The staged world could not update its active camera.");
		}
	}
	if (desc.sky)
		m_state->WriteSkyEnvironment(SkyEnvironment{.Description = *desc.sky});
	else
		m_state->RemoveSkyEnvironment();
	for (const SceneLightDesc& light : desc.lights)
	{
		if (!m_state->AddLight(SceneLightDesc(light)).IsValid())
		{
			Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "The staged world rejected a level light.");
		}
	}
}

void GameWorld::Update(float deltaSeconds)
{
	m_editCommands->Apply(m_generation, *m_state, *m_resources);
	if (m_cameraInputIntent.SpeedStepCount != 0.0f)
	{
		m_cameraNavigationSettings.MoveSpeedMetersPerSecond = CameraNavigation::ApplySpeedSteps(
		    m_cameraNavigationSettings.MoveSpeedMetersPerSecond,
		    m_cameraInputIntent.SpeedStepCount,
		    m_cameraNavigationSettings.MinimumMoveSpeedMetersPerSecond,
		    m_cameraNavigationSettings.MaximumMoveSpeedMetersPerSecond);
	}
	const ECS::GameWorldSystemExecutionContext executionContext{
	    .Resources = *m_resources,
	    .Executor = m_taskExecutor,
	    .Camera =
	        {
	            .Intent = m_cameraInputIntent,
	            .NavigationSettings = m_cameraNavigationSettings,
	            .DeltaSeconds = deltaSeconds,
	        },
	};
	if (!m_state->ExecuteSystems(executionContext))
	{
		Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "Game-system graph execution failed.");
	}
	m_cameraInputIntent.LookDeltaX = 0.0f;
	m_cameraInputIntent.LookDeltaY = 0.0f;
	m_cameraInputIntent.SpeedStepCount = 0.0f;
}

WorldEditResult GameWorld::SubmitEdit(WorldEditCommand command, std::uint64_t expectedGeneration)
{
	return m_editCommands->Submit(std::move(command), expectedGeneration, m_generation, *m_state, *m_resources);
}

void GameWorld::PublishCameraInputIntent(const CameraInputIntent& intent) noexcept
{
	m_cameraInputIntent = intent;
}

void GameWorld::CommitSceneLoadPackage(Assets::SceneLoadPackage&& package)
{
	std::size_t expectedEntityCount = 1 + package.Level.lights.size();
	for (const SceneAssetPayload& payload : package.AssetPayloads)
	{
		expectedEntityCount += payload.animations.size() + payload.staticMeshInstances.size() + payload.skeletalMeshInstances.size()
		    + payload.cameras.size() + payload.lights.size();
	}
	if (package.Entities.size() != expectedEntityCount)
	{
		Diagnostics::Fatal(
		    g_gameWorldLogger,
		    __FILE__,
		    __LINE__,
		    "Scene load blueprint count does not match the entity construction records.");
	}

	GameWorld stagedWorld(m_taskExecutor);
	stagedWorld.InitializeStagedLevel(package.Level);

	GameWorldSceneAssetCommitter committer(*stagedWorld.m_state, *stagedWorld.m_resources);
	for (SceneAssetPayload& payload : package.AssetPayloads)
	{
		committer.Commit(std::move(payload));
	}
	EntityId selectedCamera;
	for (std::size_t index = 1; index < stagedWorld.m_state->GetCameraCount(); ++index)
	{
		const std::optional<SceneCameraEntry> camera = stagedWorld.m_state->ReadCamera(stagedWorld.m_state->GetCameraEntity(index));
		if (!camera)
		{
			Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "The staged world could not read an enumerated camera.");
		}
		if (camera->IsPerspective())
		{
			selectedCamera = stagedWorld.m_state->GetCameraEntity(index);
			break;
		}
	}
	if (!selectedCamera.IsValid() && stagedWorld.m_state->GetCameraCount() != 0)
	{
		selectedCamera = stagedWorld.m_state->GetCameraEntity(0);
	}
	if (selectedCamera.IsValid() && !stagedWorld.m_state->SetActiveCamera(selectedCamera))
	{
		Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "The staged world could not activate its selected camera.");
	}
	if (!stagedWorld.m_state->PrepareSystemResources(*stagedWorld.m_resources))
	{
		Diagnostics::Fatal(g_gameWorldLogger, __FILE__, __LINE__, "Staged world could not resolve animation targets and output slots.");
	}
	if (stagedWorld.m_state->GetEntityCount() != package.Entities.size())
	{
		Diagnostics::Fatal(
		    g_gameWorldLogger,
		    __FILE__,
		    __LINE__,
		    "Staged world entity count does not match the validated blueprint package.");
	}

	m_state.swap(stagedWorld.m_state);
	m_resources.swap(stagedWorld.m_resources);
	m_activeLevelName.swap(stagedWorld.m_activeLevelName);
	std::swap(m_activeLevelDesc, stagedWorld.m_activeLevelDesc);
	++m_generation;
}

RenderFrameSubmission GameWorld::ExtractRenderFrameSubmission(std::uint64_t frameId)
{
	return m_renderFrameSubmissionExtractor->Extract(*m_state, *m_resources, AcquireReadView(), m_generation, frameId);
}

void GameWorld::FinalizeSceneLoadCommit()
{
	CommitWorldChanges();
}

bool GameWorld::IsEntityAlive(EntityId entity) const noexcept
{
	return m_state->IsAlive(entity);
}

bool GameWorld::DestroyEntity(EntityId entity) noexcept
{
	return m_state->Destroy(entity);
}

WorldReadView GameWorld::AcquireReadView() const noexcept
{
	return m_state->AcquireReadView();
}

WorldChangeBatch GameWorld::ReadChanges(const WorldChangeCursor& cursor) const
{
	return m_state->ReadChanges(cursor.m_acknowledgedSequence);
}

bool GameWorld::AcknowledgeChanges(WorldChangeCursor& cursor, WorldSequence sequence) const noexcept
{
	const WorldSequence publishedSequence = AcquireReadView().GetSequence();
	if (sequence < cursor.m_acknowledgedSequence || sequence > publishedSequence)
	{
		return false;
	}
	cursor.m_acknowledgedSequence = sequence;
	return true;
}

void GameWorld::CommitWorldChanges()
{
	m_state->CommitDerivedStateAndPublish();
}

std::size_t GameWorld::GetMaterialVariantCount() const noexcept
{
	return m_resources->MaterialVariants.GetCount();
}

std::string_view GameWorld::GetMaterialVariantName(std::size_t index) const noexcept
{
	return m_resources->MaterialVariants.GetName(index);
}

MaterialVariantIndex GameWorld::GetActiveMaterialVariant() const noexcept
{
	return m_resources->MaterialVariants.GetActive();
}

bool GameWorld::ApplyMaterialVariant(MaterialVariantIndex index)
{
	const bool applied = m_resources->MaterialVariants.Apply(index, *m_state);
	if (applied)
		CommitWorldChanges();
	return applied;
}

WorldMaterialVariantView GameWorld::CaptureMaterialVariants() const
{
	WorldMaterialVariantView view;
	view.Active = m_resources->MaterialVariants.GetActive();
	view.Names.reserve(m_resources->MaterialVariants.GetCount());
	for (std::size_t index = 0; index < m_resources->MaterialVariants.GetCount(); ++index)
	{
		view.Names.emplace_back(m_resources->MaterialVariants.GetName(index));
	}
	return view;
}
