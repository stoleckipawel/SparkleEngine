#include "PCH.h"
#include "World/GameWorld.h"

#include "Assets/SceneAssetPayload.h"
#include "World/GameWorldSceneAssetCommitter.h"
#include "World/GameWorldController.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "World/GameWorldState.h"
#include "Level/Loading/SceneLoadPackage.h"
#include "World/Resources/GameWorldResourceStores.h"

#include <algorithm>
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

	void RunControllers(
	    std::vector<std::unique_ptr<GameWorldController>>& controllers,
	    GameWorld& world,
	    const GameWorldUpdateContext& context)
	{
		for (const std::unique_ptr<GameWorldController>& controller : controllers)
		{
			if (!controller)
			{
				continue;
			}

			controller->Update(world, context);
		}
	}
}

GameWorld::GameWorld() :
    m_state(std::make_unique<ECS::GameWorldState>()),
	m_resources(std::make_unique<GameWorldResourceStores>()),
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
	const GameWorldUpdateContext preAnimationContext{.deltaSeconds = deltaSeconds, .phase = GameWorldUpdatePhase::PreAnimation};
	RunControllers(m_controllers, *this, preAnimationContext);

	m_state->UpdateAnimations(deltaSeconds, m_resources->Skeletons);
	m_state->ApplyMorphWeights(m_state->GetAnimationOutput().morphWeights);

	const GameWorldUpdateContext postAnimationContext{.deltaSeconds = deltaSeconds, .phase = GameWorldUpdatePhase::PostAnimation};
	RunControllers(m_controllers, *this, postAnimationContext);
	CommitWorldChanges();
}

void GameWorld::RegisterController(std::unique_ptr<GameWorldController>&& controller)
{
	if (!controller)
	{
		return;
	}

	if (m_activeLevelName.empty())
	{
		controller->OnWorldReset(*this);
	}
	else
	{
		controller->OnLevelLoaded(*this, m_activeLevelDesc);
		controller->OnSceneAssetsAppended(*this);
	}

	m_controllers.push_back(std::move(controller));
	CommitWorldChanges();
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

	GameWorld stagedWorld;
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
	for (const std::unique_ptr<GameWorldController>& controller : m_controllers)
	{
		if (!controller)
			continue;
		controller->OnWorldReset(*this);
		controller->OnLevelLoaded(*this, m_activeLevelDesc);
		controller->OnSceneAssetsAppended(*this);
	}
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
