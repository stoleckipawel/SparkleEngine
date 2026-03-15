#include "PCH.h"

#include "SceneRenderStateCoordinator.h"

#include "D3D12Rhi.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Runtime/Level/LevelChangeEvents.h"
#include "Scene/Scene.h"
#include "SceneData/MaterialCacheManager.h"
#include "SceneData/RenderSceneSnapshotCache.h"
#include "TextureManager.h"

SceneRenderStateCoordinator::SceneRenderStateCoordinator(
    LevelChangeEvents& levelChangeEvents,
    Scene& scene,
    D3D12Rhi& rhi,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager,
    RenderCamera& renderCamera,
    RenderSceneSnapshotCache& renderSceneSnapshotCache,
    MaterialCacheManager& materialCache) noexcept :
    m_scene(&scene),
    m_rhi(&rhi),
    m_gpuMeshCache(&gpuMeshCache),
    m_textureManager(&textureManager),
    m_renderCamera(&renderCamera),
    m_renderSceneSnapshotCache(&renderSceneSnapshotCache),
    m_materialCache(&materialCache)
{
	SubscribeToLevelLifecycleEvents(levelChangeEvents);
}

void SceneRenderStateCoordinator::SubscribeToLevelLifecycleEvents(LevelChangeEvents& levelChangeEvents) noexcept
{
	auto willUnloadHandle = levelChangeEvents.OnLevelWillUnload.Add(
	    [this](const LevelWillUnloadEventArgs&)
	    {
		    OnLevelWillUnload();
	    });
	m_levelWillUnloadHandle = ScopedEventHandle(levelChangeEvents.OnLevelWillUnload, willUnloadHandle);

	auto changedHandle = levelChangeEvents.OnLevelChanged.Add(
	    [this](const LevelChangedEventArgs&)
	    {
		    OnLevelChanged();
	    });
	m_levelChangedHandle = ScopedEventHandle(levelChangeEvents.OnLevelChanged, changedHandle);
}

void SceneRenderStateCoordinator::OnLevelWillUnload() noexcept
{
	InvalidateSceneScopedRendererState();
}

void SceneRenderStateCoordinator::OnLevelChanged() noexcept
{
	RefreshSceneScopedRendererState();
}

void SceneRenderStateCoordinator::InvalidateSceneScopedRendererState() noexcept
{
	if (m_rhi)
	{
		m_rhi->Flush();
	}

	if (m_gpuMeshCache)
	{
		m_gpuMeshCache->Clear();
	}

	ReleaseSceneScopedMaterialResources();
}

void SceneRenderStateCoordinator::RefreshSceneScopedRendererState() noexcept
{
	if (m_renderCamera)
	{
		m_renderCamera->ForceUpdate();
	}

	if (m_scene && m_renderSceneSnapshotCache && m_materialCache)
	{
		const auto& sceneSnapshot = m_renderSceneSnapshotCache->Capture(*m_scene);
		m_materialCache->Rebuild(sceneSnapshot);
	}
}

void SceneRenderStateCoordinator::ReleaseSceneScopedMaterialResources() noexcept
{
	if (m_renderSceneSnapshotCache)
	{
		m_renderSceneSnapshotCache->Reset();
	}

	if (m_materialCache)
	{
		m_materialCache->Reset();
	}

	if (m_textureManager)
	{
		m_textureManager->UnloadSceneTextures();
	}
}