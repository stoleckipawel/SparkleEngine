#include "PCH.h"
#include "SceneData/Input/RenderInputConsumer.h"

#include "Meshes/GPUMeshCache.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/RenderWorld.h"
#include "Textures/TextureManager.h"

RenderInputConsumer::RenderInputConsumer(
    RenderWorld& world,
    RenderDeviceServices& backend,
    GPUMeshCache& meshCache,
    TextureManager& textureManager,
    MaterialCacheManager& materialCache,
    RenderRayTracingScene* rayTracingScene) noexcept :
    m_world(&world),
    m_backend(&backend),
    m_meshCache(&meshCache),
    m_textureManager(&textureManager),
    m_materialCache(&materialCache),
    m_rayTracingScene(rayTracingScene)
{
}

RenderInputConsumeResult RenderInputConsumer::ConsumePending() noexcept
{
	if (!m_pending) return {};
	RenderInputFrame input = std::move(*m_pending);
	m_pending.reset();
	RenderInputConsumeResult result;
	const RenderWorldApplyStatus status = m_world->Apply(input.WorldDelta, result.Diagnostic);
	if (status != RenderWorldApplyStatus::Applied) return result;
	result.Accepted = true;
	result.SceneReset = input.WorldDelta.ResetScene;
	if (result.SceneReset) ResetSceneResources();
	m_dynamic = std::move(input.Dynamic);
	return result;
}

void RenderInputConsumer::ResetSceneResources() noexcept
{
	m_backend->WaitForIdle();
	if (m_rayTracingScene) m_rayTracingScene->Clear();
	m_meshCache->Clear();
	m_materialCache->Reset();
	m_textureManager->UnloadSceneTextures();
}
