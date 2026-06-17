#include "PCH.h"
#include "Scene/GameScene.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/GameSceneAssetPayloadAppender.h"
#include "Scene/GameSceneController.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"

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

GameScene::GameScene() = default;

GameScene::~GameScene() noexcept = default;

GameSceneLoadResult GameScene::LoadLevel(const LevelAsset& level)
{
	return LoadLevel(level.BuildDescription());
}

GameSceneLoadResult GameScene::LoadLevel(const LevelDesc& desc)
{
	GameSceneLoadResult result;

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Loading level '{}'", desc.name);

	Clear();
	m_activeLevelName = desc.name;
	m_activeLevelDesc = desc;
	m_cameras.Reset(desc.cameraDesc);
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

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Level '{}' loaded", desc.name);
	return result;
}

bool GameScene::AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload)
{
	const SceneAssetPayloadDiagnostics diagnostics = sceneAssetPayload.diagnostics;
	GameSceneAssetPayloadAppender appender(
	    m_cameras,
	    m_lighting,
	    m_materials,
	    m_materialVariants,
	    m_meshes,
	    m_skeletons,
	    m_animations,
	    m_textures);
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

	SPDLOG_LOGGER_INFO(
	    g_gameSceneLogger,
	    "Scene: Loaded {} meshes, {} materials, {} material variants, {} skeletons, {} animation clips, payload sceneAssets={}, meshAssetRefs={}, meshInstances={}, instanceGroups={}, variantMappings={}, cameras={}, lights={}, skeletonRefs={}, animationRefs={}, featureFlags=0x{:08X}",
	    m_meshes.GetMeshCount(),
	    m_materials.GetMaterialCount(),
	    m_materialVariants.GetVariantCount(),
	    m_skeletons.GetSkeletonCount(),
	    m_animations.GetClipCount(),
	    diagnostics.loadedSceneAssetCount,
	    diagnostics.meshAssetReferenceCount,
	    diagnostics.meshInstanceCount,
	    diagnostics.meshInstanceGroupCount,
	    diagnostics.materialVariantMappingCount,
	    diagnostics.cameraCount,
	    diagnostics.lightCount,
	    diagnostics.skeletonRefCount,
	    diagnostics.animationRefCount,
	    diagnostics.sceneFeatureFlags);

	return true;
}

void GameScene::Update(float deltaSeconds)
{
	const GameSceneUpdateContext preAnimationContext{
	    .deltaSeconds = deltaSeconds,
	    .phase = GameSceneUpdatePhase::PreAnimation};
	RunControllers(m_controllers, *this, preAnimationContext);

	m_animations.Update(deltaSeconds, m_skeletons);
	m_meshes.ApplyMorphWeights(m_animations.GetActiveMorphWeights());

	const GameSceneUpdateContext postAnimationContext{
	    .deltaSeconds = deltaSeconds,
	    .phase = GameSceneUpdatePhase::PostAnimation};
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
	snapshot.camera = m_cameras.GetActiveCamera().CaptureSnapshot();
	snapshot.animations = m_animations.CaptureSnapshot();
	snapshot.lighting = m_lighting.CaptureSnapshot();
	snapshot.textures = m_textures.CaptureSnapshot();
	snapshot.materials = m_materials.CaptureSnapshot();
	snapshot.meshes = m_meshes.CaptureSnapshot();
	return snapshot;
}

void GameScene::Clear()
{
	m_activeLevelName.clear();
	m_activeLevelDesc = {};
	m_lighting.Reset();
	m_materials.Reset();
	m_materialVariants.Reset();
	m_meshes.Reset();
	m_skeletons.Clear();
	m_animations.Clear();
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
