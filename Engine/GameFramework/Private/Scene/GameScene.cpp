#include "PCH.h"
#include "Scene/GameScene.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/GameSceneAssetPayloadAppender.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Scene/Meshes/MeshComponent.h"

#include <algorithm>
#include <cmath>
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
	m_activeLevelName = desc.name;
	m_ptlasShowcaseMotionEnabled = desc.name == "Sponza";
	m_ptlasShowcaseTimeSeconds = 0.0f;
	m_cameras.Reset(desc.cameraDesc);
	m_lighting.ApplyFromDesc(desc.lights);

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
	if (m_ptlasShowcaseMotionEnabled)
	{
		CapturePtlasShowcaseBaseTransforms();
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
	m_animations.Update(deltaSeconds, m_skeletons);
	m_meshes.ApplyMorphWeights(m_animations.GetActiveMorphWeights());
	UpdatePtlasShowcaseMotion(deltaSeconds);
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
	m_ptlasShowcaseTimeSeconds = 0.0f;
	m_ptlasShowcaseMotionEnabled = false;
	m_ptlasShowcaseAnimatedMeshes.clear();
	m_lighting.Reset();
	m_materials.Reset();
	m_materialVariants.Reset();
	m_meshes.Reset();
	m_skeletons.Clear();
	m_animations.Clear();
	m_textures.Reset();
	m_cameras.Reset();
}

void GameScene::CapturePtlasShowcaseBaseTransforms() noexcept
{
	m_ptlasShowcaseAnimatedMeshes.clear();
	for (std::size_t meshIndex = 0; meshIndex < m_meshes.GetMeshCount(); ++meshIndex)
	{
		const MeshComponent* mesh = m_meshes.GetMeshComponent(meshIndex);
		if (mesh == nullptr || !mesh->IsSkeletalMeshComponent())
		{
			continue;
		}

		m_ptlasShowcaseAnimatedMeshes.push_back(PtlasShowcaseAnimatedMesh{
		    .MeshIndex = meshIndex,
		    .BaseTransform = mesh->GetTransform()});
	}
}

void GameScene::UpdatePtlasShowcaseMotion(float deltaSeconds) noexcept
{
	if (!m_ptlasShowcaseMotionEnabled || m_ptlasShowcaseAnimatedMeshes.empty())
	{
		return;
	}

	m_ptlasShowcaseTimeSeconds += (std::max)(0.0f, deltaSeconds);
	const float phase = m_ptlasShowcaseTimeSeconds * 0.7f;
	const float aisleX = std::sin(phase) * 3.25f;
	const float facingYaw = DirectX::XM_PIDIV2;
	const DirectX::XMMATRIX cesiumManUprightBasisCorrection = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
	const DirectX::XMMATRIX walkMotion =
	    cesiumManUprightBasisCorrection *
	    DirectX::XMMatrixRotationY(facingYaw) *
	    DirectX::XMMatrixTranslation(aisleX, 0.0f, 0.0f);

	for (const PtlasShowcaseAnimatedMesh& animatedMesh : m_ptlasShowcaseAnimatedMeshes)
	{
		MeshComponent* mesh = m_meshes.GetMeshComponent(animatedMesh.MeshIndex);
		if (mesh == nullptr)
		{
			continue;
		}

		mesh->SetTransform(Transform(animatedMesh.BaseTransform.GetWorldMatrix() * walkMotion));
	}
}
