#include "PCH.h"

#include "SceneRenderStateCoordinator.h"

#include "Meshes/GPUMeshCache.h"
#include "Camera/RenderCamera.h"
#include "Level/LevelChangeEvents.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Scene/GameScene.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "Textures/TextureManager.h"

SceneRenderStateCoordinator::SceneRenderStateCoordinator(
    LevelChangeEvents& levelChangeEvents,
    GameScene& gameScene,
    RenderDeviceServices& backendServices,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager,
    RenderSceneSnapshot& sceneSnapshot,
    RenderCamera& renderCamera,
    MaterialCacheManager& materialCache) noexcept :
    m_gameScene(&gameScene),
    m_backendServices(&backendServices),
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
	if (m_backendServices)
	{
		m_backendServices->Flush();
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
		m_sceneSnapshot->Capture(m_gameScene->CaptureSnapshot());
	}

	if (m_renderCamera && m_sceneSnapshot)
	{
		m_renderCamera->ForceUpdate(m_sceneSnapshot->camera);
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