#include "PCH.h"

#include "SceneRenderStateCoordinator.h"

#include "Meshes/GPUMeshCache.h"
#include "Camera/RenderCamera.h"
#include "Level/LevelChangeEvents.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "World/GameWorld.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "Textures/TextureManager.h"

SceneRenderStateCoordinator::SceneRenderStateCoordinator(
    LevelChangeEvents& levelChangeEvents,
    GameWorld& gameWorld,
    RenderDeviceServices& backendServices,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager,
    RenderCamera& renderCamera,
    MaterialCacheManager& materialCache,
    RenderRayTracingScene& rayTracingScene) noexcept :
    m_gameWorld(&gameWorld),
    m_backendServices(&backendServices),
    m_gpuMeshCache(&gpuMeshCache),
    m_textureManager(&textureManager),
    m_renderCamera(&renderCamera),
    m_materialCache(&materialCache),
    m_rayTracingScene(&rayTracingScene)
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
	m_temporalHistoryResetRequested = true;
	m_temporalHistoryResetReason = "Level will unload";
	InvalidateSceneScopedRendererState();
}

void SceneRenderStateCoordinator::OnLevelChanged() noexcept
{
	m_temporalHistoryResetRequested = true;
	m_temporalHistoryResetReason = "Level changed";
	RefreshSceneScopedRendererState();
}

bool SceneRenderStateCoordinator::ConsumeTemporalHistoryResetRequest(std::string& outReason) noexcept
{
	if (!m_temporalHistoryResetRequested)
	{
		return false;
	}

	outReason = m_temporalHistoryResetReason;
	m_temporalHistoryResetRequested = false;
	m_temporalHistoryResetReason.clear();
	return true;
}

void SceneRenderStateCoordinator::InvalidateSceneScopedRendererState() noexcept
{
	if (m_backendServices)
	{
		m_backendServices->WaitForIdle();
	}

	if (m_rayTracingScene)
	{
		m_rayTracingScene->Clear();
	}
	if (m_gpuMeshCache)
	{
		m_gpuMeshCache->Clear();
	}

	ReleaseSceneScopedMaterialResources();
}

void SceneRenderStateCoordinator::RefreshSceneScopedRendererState() noexcept
{
	if (m_gameWorld && m_renderCamera)
	{
		m_renderCamera->ForceUpdate(m_gameWorld->CaptureSnapshot().camera);
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
