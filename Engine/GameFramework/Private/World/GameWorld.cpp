#include "PCH.h"
#include "World/GameWorld.h"

#include "Assets/SceneAssetPayload.h"
#include "World/GameWorldSceneAssetCommitter.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "World/GameWorldState.h"
#include "Level/Loading/SceneLoadPackage.h"
#include "World/Resources/GameWorldResourceStores.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
	CameraSnapshot BuildCameraSnapshot(const WorldReadView& readView) noexcept
	{
		for (const WorldCameraReadData& camera : readView.GetCameras())
		{
			if (!camera.Active)
			{
				continue;
			}
			return CameraSnapshot{
			    .position = camera.LocalTransform.GetTranslation(),
			    .direction = camera.Direction,
			    .fovYDegrees = camera.Description.fovYDegrees,
			    .aspectRatio = camera.AspectRatio,
			    .nearZ = camera.Description.nearZ,
			    .farZ = camera.Description.farZ};
		}
		return {};
	}

}

GameWorld::GameWorld(TaskExecutor& taskExecutor) :
    m_state(std::make_unique<ECS::GameWorldState>()),
	m_resources(std::make_unique<GameWorldResourceStores>()),
	m_taskExecutor(taskExecutor),
    m_cameras(*this),
    m_lighting(*this),
    m_meshes(*this),
    m_sky(*this)
{
}

GameWorld::~GameWorld() noexcept = default;

void GameWorld::InitializeStagedLevel(const LevelDesc& desc)
{
	m_activeLevelName = desc.name;
	m_activeLevelDesc = desc;
	m_cameras.Reset(desc.cameraDesc);
	m_sky.ApplyFromDesc(desc.sky);
	m_lighting.ApplyFromDesc(desc.lights);
}

void GameWorld::Update(float deltaSeconds)
{
	if (!m_state->ExecuteSystems(*m_resources, m_taskExecutor, m_cameraInputIntent, deltaSeconds))
		throw std::runtime_error("Game-system graph execution failed.");
	m_cameraInputIntent.LookDeltaX = 0.0f;
	m_cameraInputIntent.LookDeltaY = 0.0f;
	m_cameraInputIntent.SpeedStepCount = 0.0f;
}

void GameWorld::PublishCameraInputIntent(const CameraInputIntent& intent) noexcept
{
	m_cameraInputIntent = intent;
}

void GameWorld::EnableOscillatingMeshMotion(bool enabled)
{
	m_oscillatingMeshMotionEnabled = enabled;
	m_state->ConfigureOscillatingMeshMotion(enabled);
}

bool GameWorld::CommitSceneLoadPackage(Assets::SceneLoadPackage&& package, std::string& errorMessage)
{
	std::size_t expectedEntityCount = 1 + package.Level.lights.size();
	for (const SceneAssetPayload& payload : package.AssetPayloads)
	{
		expectedEntityCount += payload.animations.size() + payload.staticMeshInstances.size() +
		                       payload.skeletalMeshInstances.size() + payload.cameras.size() + payload.lights.size();
	}
	if (package.Entities.size() != expectedEntityCount)
	{
		errorMessage = "Scene load blueprint count does not match the entity construction records.";
		return false;
	}

	GameWorld stagedWorld(m_taskExecutor);
	stagedWorld.m_oscillatingMeshMotionEnabled = m_oscillatingMeshMotionEnabled;
	stagedWorld.m_state->ConfigureOscillatingMeshMotion(m_oscillatingMeshMotionEnabled);
	stagedWorld.InitializeStagedLevel(package.Level);

	GameWorldSceneAssetCommitter committer(
	    *stagedWorld.m_state,
	    *stagedWorld.m_resources);
	for (SceneAssetPayload& payload : package.AssetPayloads)
	{
		if (!committer.Commit(std::move(payload)))
		{
			errorMessage = "GameWorld rejected a validated scene asset payload.";
			return false;
		}
	}
	stagedWorld.GetCameras().SetPrimaryCameraActive();
	if (!stagedWorld.m_state->PrepareSystemResources(*stagedWorld.m_resources))
	{
		errorMessage = "Staged world could not resolve animation targets and output slots.";
		return false;
	}
	if (stagedWorld.m_state->GetEntityCount() != package.Entities.size())
	{
		errorMessage = "Staged world entity count does not match the validated blueprint package.";
		return false;
	}

	m_state.swap(stagedWorld.m_state);
	m_resources.swap(stagedWorld.m_resources);
	m_activeLevelName.swap(stagedWorld.m_activeLevelName);
	std::swap(m_activeLevelDesc, stagedWorld.m_activeLevelDesc);
	++m_generation;

	errorMessage.clear();
	return true;
}

void GameWorld::FinalizeSceneLoadCommit()
{
	CommitWorldChanges();
}

GameWorldSnapshot GameWorld::CaptureSnapshot() const
{
	GameWorldSnapshot snapshot;
	snapshot.camera = BuildCameraSnapshot(AcquireReadView());
	snapshot.animations = m_state->GetAnimationOutput();
	snapshot.lighting = m_lighting.CaptureSnapshot();
	snapshot.sky = m_sky.CaptureSnapshot();
	const std::optional<SceneSkyDesc> sky = m_sky.GetSky();
	if (sky && sky->skyTexture.IsValid())
	{
		const std::filesystem::path skyTexturePath(sky->skyTexture.texturePath);
		snapshot.textures = m_resources->Textures.CaptureSnapshot(std::span<const std::filesystem::path>(&skyTexturePath, 1));
	}
	else
	{
		snapshot.textures = m_resources->Textures.CaptureSnapshot();
	}
	snapshot.materials = m_resources->Materials.CaptureSnapshot();
	snapshot.meshes = m_meshes.CaptureSnapshot();
	return snapshot;
}

bool GameWorld::IsEntityAlive(EntityId entity) const noexcept { return m_state->IsAlive(entity); }

bool GameWorld::DestroyEntity(EntityId entity) noexcept { return m_state->Destroy(entity); }

WorldReadView GameWorld::AcquireReadView() const noexcept { return m_state->AcquireReadView(); }

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

void GameWorld::CommitWorldChanges() { m_state->CommitDerivedStateAndPublish(); }

std::size_t GameWorld::GetMaterialVariantCount() const noexcept { return m_resources->MaterialVariants.GetCount(); }

std::string_view GameWorld::GetMaterialVariantName(std::size_t index) const noexcept
{
	return m_resources->MaterialVariants.GetName(index);
}

MaterialVariantIndex GameWorld::GetActiveMaterialVariant() const noexcept { return m_resources->MaterialVariants.GetActive(); }

bool GameWorld::ApplyMaterialVariant(MaterialVariantIndex index)
{
	const bool applied = m_resources->MaterialVariants.Apply(index, *m_state);
	if (applied)
		CommitWorldChanges();
	return applied;
}
