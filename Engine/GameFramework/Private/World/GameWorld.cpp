#include "PCH.h"
#include "World/GameWorld.h"

#include "Assets/SceneAssetPayload.h"
#include "World/GameWorldAssetPayloadAppender.h"
#include "World/GameWorldController.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "World/GameWorldState.h"

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
    m_cameras(*this),
    m_lighting(*this),
    m_meshes(*this),
    m_sky(*this)
{
}

GameWorld::~GameWorld() noexcept = default;

GameWorldLoadResult GameWorld::LoadLevel(const LevelAsset& level)
{
	return LoadLevel(level.BuildDescription());
}

GameWorldLoadResult GameWorld::LoadLevel(const LevelDesc& desc)
{
	GameWorldLoadResult result;
	Clear();
	m_activeLevelName = desc.name;
	m_activeLevelDesc = desc;
	m_cameras.Reset(desc.cameraDesc);
	m_sky.ApplyFromDesc(desc.sky);
	m_lighting.ApplyFromDesc(desc.lights);
	for (const std::unique_ptr<GameWorldController>& controller : m_controllers)
	{
		if (!controller)
		{
			continue;
		}

		controller->OnLevelLoaded(*this, m_activeLevelDesc);
	}
	CommitWorldChanges();

	result.status = GameWorldLoadStatus::Succeeded;

	return result;
}

bool GameWorld::AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload)
{
	GameWorldAssetPayloadAppender
	    appender(m_cameras, m_lighting, m_materials, m_materialVariants, m_meshes, m_skeletons, *m_state, m_textures);
	if (!appender.Append(std::move(sceneAssetPayload)))
	{
		return false;
	}

	for (const std::unique_ptr<GameWorldController>& controller : m_controllers)
	{
		if (!controller)
		{
			continue;
		}

		controller->OnSceneAssetsAppended(*this);
	}
	CommitWorldChanges();

	return true;
}

void GameWorld::Update(float deltaSeconds)
{
	const GameWorldUpdateContext preAnimationContext{.deltaSeconds = deltaSeconds, .phase = GameWorldUpdatePhase::PreAnimation};
	RunControllers(m_controllers, *this, preAnimationContext);

	m_state->UpdateAnimations(deltaSeconds, m_skeletons);
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
		snapshot.textures = m_textures.CaptureSnapshot(std::span<const std::filesystem::path>(&skyTexturePath, 1));
	}
	else
	{
		snapshot.textures = m_textures.CaptureSnapshot();
	}
	snapshot.materials = m_materials.CaptureSnapshot();
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

void GameWorld::Clear()
{
	m_state->Clear();
	m_activeLevelName.clear();
	m_activeLevelDesc = {};
	m_materials.Reset();
	m_materialVariants.Reset();
	m_skeletons.Clear();
	m_sky.Reset();
	m_textures.Reset();
	m_cameras.Reset();

	for (const std::unique_ptr<GameWorldController>& controller : m_controllers)
	{
		if (!controller)
		{
			continue;
		}

		controller->OnWorldReset(*this);
	}
	CommitWorldChanges();
}
