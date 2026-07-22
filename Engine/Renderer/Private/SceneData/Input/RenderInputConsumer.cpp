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

bool RenderInputConsumer::Submit(RenderInputFrame input)
{
	if (m_pending) return false;
	m_pending = std::move(input);
	return true;
}

RenderInputConsumeResult RenderInputConsumer::ConsumePending() noexcept
{
	RenderInputConsumeResult result;
	if (!m_pending) return result;
	RenderInputFrame input = std::move(*m_pending);
	m_pending.reset();
	if (m_world->Validate(input.WorldDelta, result.Diagnostic) != RenderWorldApplyStatus::Applied) return result;

	bool historyResetRequired = false;
	if (!m_validator.Validate(*m_world, input, historyResetRequired, result.Diagnostic)) return result;
	if (m_world->Apply(input.WorldDelta, result.Diagnostic) != RenderWorldApplyStatus::Applied) return result;

	input.Dynamic.Metadata.ResetHistory |= historyResetRequired;
	m_validator.Commit(input.Dynamic.Metadata);
	result.Accepted = true;
	result.SceneReset = input.WorldDelta.ResetScene;
	m_dynamic = std::move(input.Dynamic);
	if (result.SceneReset) ResetSceneResources();
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
