#include "PCH.h"

#include "SceneRenderStateCoordinator.h"

#include "D3D12Rhi.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Runtime/Level/LevelChangeEvents.h"
#include "SceneData/RendererMaterialCacheState.h"
#include "TextureManager.h"

SceneRenderStateCoordinator::SceneRenderStateCoordinator(
	LevelChangeEvents& levelChangeEvents,
	D3D12Rhi& rhi,
	GPUMeshCache& gpuMeshCache,
	TextureManager& textureManager,
	RenderCamera& renderCamera,
	RendererMaterialCacheState& materialCache,
	SceneRenderStateCoordinatorCallbacks callbacks) noexcept :
	m_rhi(&rhi),
	m_gpuMeshCache(&gpuMeshCache),
	m_textureManager(&textureManager),
	m_renderCamera(&renderCamera),
	m_materialCache(&materialCache),
	m_callbacks(callbacks)
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

	if (m_callbacks.rebuildSceneResources)
	{
		m_callbacks.rebuildSceneResources(m_callbacks.context);
	}
}

void SceneRenderStateCoordinator::ReleaseSceneScopedMaterialResources() noexcept
{
	if (m_callbacks.releaseMaterialTextureTables)
	{
		m_callbacks.releaseMaterialTextureTables(m_callbacks.context);
	}

	if (m_textureManager)
	{
		m_textureManager->UnloadSceneTextures();
	}

	ResetMaterialCacheState();
}

void SceneRenderStateCoordinator::ResetMaterialCacheState() noexcept
{
	if (!m_materialCache)
	{
		return;
	}

	m_materialCache->cachedMaterialData.clear();
	m_materialCache->cachedMaterialDescs.clear();
	m_materialCache->materialCacheBuilt = false;
	m_materialCache->materialCacheUsesLoadedMaterials = false;
}