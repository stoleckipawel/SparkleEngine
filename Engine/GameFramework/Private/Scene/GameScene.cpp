#include "PCH.h"
#include "Scene/GameScene.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/GameSceneAssetPayloadAppender.h"
#include "Scene/GameSceneController.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "World/SceneWorld.h"

#include <algorithm>
#include <utility>

static const auto g_gameSceneLogger = Logging::GetOrCreateLogger("GameFramework.GameScene");

namespace
{
	void RunControllers(
	    std::vector<std::unique_ptr<GameSceneController>>& controllers,
	    GameScene& scene,
	    const GameSceneUpdateContext& context)
	{
		for (const std::unique_ptr<GameSceneController>& controller : controllers)
		{
			if (!controller)
			{
				continue;
			}

			controller->Update(scene, context);
		}
	}
}

GameScene::GameScene() :
    m_world(std::make_unique<ECS::SceneWorld>()),
    m_cameras(*this),
    m_lighting(*this),
    m_meshes(*this)
{
}

GameScene::~GameScene() noexcept = default;

GameSceneLoadResult GameScene::LoadLevel(const LevelAsset& level)
{
	return LoadLevel(level.BuildDescription());
}

GameSceneLoadResult GameScene::LoadLevel(const LevelDesc& desc)
{
	GameSceneLoadResult result;


	Clear();
	m_activeLevelName = desc.name;
	m_activeLevelDesc = desc;
	m_cameras.Reset(desc.cameraDesc);
	m_sky.ApplyFromDesc(desc.sky);
	m_lighting.ApplyFromDesc(desc.lights);
	for (const std::unique_ptr<GameSceneController>& controller : m_controllers)
	{
		if (!controller)
		{
			continue;
		}

		controller->OnLevelLoaded(*this, m_activeLevelDesc);
	}

	result.status = GameSceneLoadStatus::Succeeded;

	return result;
}

bool GameScene::AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload)
{
	GameSceneAssetPayloadAppender
	    appender(m_cameras, m_lighting, m_materials, m_materialVariants, m_meshes, m_skeletons, *m_world, m_textures);
	if (!appender.Append(std::move(sceneAssetPayload)))
	{
		return false;
	}

	for (const std::unique_ptr<GameSceneController>& controller : m_controllers)
	{
		if (!controller)
		{
			continue;
		}

		controller->OnSceneAssetsAppended(*this);
	}

	return true;
}

void GameScene::Update(float deltaSeconds)
{
	const GameSceneUpdateContext preAnimationContext{.deltaSeconds = deltaSeconds, .phase = GameSceneUpdatePhase::PreAnimation};
	RunControllers(m_controllers, *this, preAnimationContext);

	m_world->UpdateAnimations(deltaSeconds, m_skeletons);
	m_world->ApplyMorphWeights(m_world->GetAnimationOutput().morphWeights);

	const GameSceneUpdateContext postAnimationContext{.deltaSeconds = deltaSeconds, .phase = GameSceneUpdatePhase::PostAnimation};
	RunControllers(m_controllers, *this, postAnimationContext);
}

void GameScene::RegisterController(std::unique_ptr<GameSceneController>&& controller)
{
	if (!controller)
	{
		return;
	}

	if (m_activeLevelName.empty())
	{
		controller->OnSceneReset(*this);
	}
	else
	{
		controller->OnLevelLoaded(*this, m_activeLevelDesc);
		controller->OnSceneAssetsAppended(*this);
	}

	m_controllers.push_back(std::move(controller));
}

GameSceneSnapshot GameScene::CaptureSnapshot() const
{
	GameSceneSnapshot snapshot;
	snapshot.camera = m_world->CaptureCamera(m_world->GetActiveCamera());
	snapshot.animations = m_world->GetAnimationOutput();
	snapshot.lighting = m_lighting.CaptureSnapshot();
	snapshot.sky = m_sky.CaptureSnapshot();
	if (const SceneSkyDesc* sky = m_sky.GetSky(); sky != nullptr && sky->skyTexture.IsValid())
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

bool GameScene::IsEntityAlive(EntityId entity) const noexcept { return m_world->IsAlive(entity); }

bool GameScene::DestroyEntity(EntityId entity) noexcept { return m_world->Destroy(entity); }

void GameScene::Clear()
{
	m_world->Clear();
	m_activeLevelName.clear();
	m_activeLevelDesc = {};
	m_materials.Reset();
	m_materialVariants.Reset();
	m_skeletons.Clear();
	m_sky.Reset();
	m_textures.Reset();
	m_cameras.Reset();

	for (const std::unique_ptr<GameSceneController>& controller : m_controllers)
	{
		if (!controller)
		{
			continue;
		}

		controller->OnSceneReset(*this);
	}
}
