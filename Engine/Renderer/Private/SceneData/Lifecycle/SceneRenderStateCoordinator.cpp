#include "PCH.h"

#include "SceneRenderStateCoordinator.h"

#include "D3D12Rhi.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Level/LevelChangeEvents.h"
#include "Scene/GameScene.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "Renderer/Public/Textures/TextureManager.h"

SceneRenderStateCoordinator::SceneRenderStateCoordinator(
    LevelChangeEvents& levelChangeEvents,
    GameScene& gameScene,
    D3D12Rhi& rhi,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager,
    RenderSceneSnapshot& sceneSnapshot,
    RenderCamera& renderCamera,
    MaterialCacheManager& materialCache) noexcept :
    m_gameScene(&gameScene),
    m_rhi(&rhi),
    m_gpuMeshCache(&gpuMeshCache),
    m_textureManager(&textureManager),
    m_sceneSnapshot(&sceneSnapshot),
    m_renderCamera(&renderCamera),
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

	if (m_sceneSnapshot)
	{
		m_sceneSnapshot->Reset();
	}

	ReleaseSceneScopedMaterialResources();
}

void SceneRenderStateCoordinator::RefreshSceneScopedRendererState() noexcept
{
	if (m_gameScene && m_sceneSnapshot)
	{
		m_sceneSnapshot->Capture(*m_gameScene);
	}

	if (m_renderCamera && m_sceneSnapshot)
	{
		m_renderCamera->ForceUpdate(m_sceneSnapshot->camera);
	}

	if (m_sceneSnapshot && m_textureManager)
	{
		m_textureManager->LoadSceneTextures(m_sceneSnapshot->textures);
	}

	if (m_sceneSnapshot && m_materialCache)
	{
		m_materialCache->Rebuild(m_sceneSnapshot->materials);
	}
}

void SceneRenderStateCoordinator::ReleaseSceneScopedMaterialResources() noexcept
{
	if (m_materialCache)
	{
		m_materialCache->Reset();
	}

	if (m_textureManager)
	{
		m_textureManager->UnloadSceneTextures();
	}
}