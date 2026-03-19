#include "PCH.h"

#include "SceneRenderStateCoordinator.h"

#include "D3D12Rhi.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Runtime/Level/LevelChangeEvents.h"
#include "Scene/GameScene.h"
#include "SceneData/MaterialCacheManager.h"
#include "TextureManager.h"

SceneRenderStateCoordinator::SceneRenderStateCoordinator(
    LevelChangeEvents& levelChangeEvents,
    GameScene& gameScene,
    D3D12Rhi& rhi,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager,
    RenderCamera& renderCamera,
    MaterialCacheManager& materialCache) noexcept :
    m_gameScene(&gameScene),
    m_rhi(&rhi),
    m_gpuMeshCache(&gpuMeshCache),
    m_textureManager(&textureManager),
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

	ReleaseSceneScopedMaterialResources();
}

void SceneRenderStateCoordinator::RefreshSceneScopedRendererState() noexcept
{
	if (m_renderCamera)
	{
		m_renderCamera->ForceUpdate();
	}

	if (m_gameScene && m_materialCache)
	{
		m_materialCache->Rebuild(*m_gameScene);
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