#include "PCH.h"
#include "Scene/GameScene.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/GameSceneAssetPayloadAppender.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"

#include <utility>

static const auto g_gameSceneLogger = Logging::GetOrCreateLogger("GameFramework.GameScene");

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
	m_cameras.Reset(desc.cameraDesc);
	m_lighting.ApplyFromDesc(desc.lightingDesc);

	result.status = GameSceneLoadStatus::Succeeded;

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Level '{}' loaded", desc.name);
	return result;
}

bool GameScene::AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload)
{
	const SceneAssetPayloadDiagnostics diagnostics = sceneAssetPayload.diagnostics;
	GameSceneAssetPayloadAppender appender(m_cameras, m_lighting, m_materials, m_meshes, m_skeletons, m_animations, m_textures);
	if (!appender.Append(std::move(sceneAssetPayload)))
	{
		return false;
	}

	SPDLOG_LOGGER_INFO(
	    g_gameSceneLogger,
	    "Scene: Loaded {} meshes, {} materials, {} skeletons, {} animation clips, payload sceneAssets={}, meshAssetRefs={}, meshInstances={}, instanceGroups={}, cameras={}, lights={}, skeletonRefs={}, animationRefs={}, featureFlags=0x{:08X}",
	    m_meshes.GetMeshCount(),
	    m_materials.GetMaterialCount(),
	    m_skeletons.GetSkeletonCount(),
	    m_animations.GetClipCount(),
	    diagnostics.loadedSceneAssetCount,
	    diagnostics.meshAssetReferenceCount,
	    diagnostics.meshInstanceCount,
	    diagnostics.meshInstanceGroupCount,
	    diagnostics.cameraCount,
	    diagnostics.lightCount,
	    diagnostics.skeletonRefCount,
	    diagnostics.animationRefCount,
	    diagnostics.sceneFeatureFlags);

	return true;
}

void GameScene::Update(float deltaSeconds)
{
	m_animations.Update(deltaSeconds, m_skeletons);
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
	m_lighting.Reset();
	m_materials.Reset();
	m_meshes.Reset();
	m_skeletons.Clear();
	m_animations.Clear();
	m_textures.Reset();
	m_cameras.Reset();
}
