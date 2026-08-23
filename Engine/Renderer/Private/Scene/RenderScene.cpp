#include "PCH.h"
#include "Scene/RenderScene.h"

#include "Meshes/GpuMeshCache.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/GpuScene/RenderGpuScene.h"
#include "Scene/Materials/MaterialCache.h"
#include "Scene/GpuScene/GpuSceneSlotAllocator.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"

#include <utility>

RenderScene::RenderScene(
    RhiCommandSubmissionService* submissionService,
    GpuMeshCache& gpuMeshCache,
    TextureCache& textureCache,
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& rayTracingCapabilities) :
    m_gpuSceneSlots(std::make_unique<GpuSceneSlotAllocator>(submissionService)),
    m_renderGpuScene(std::make_unique<RenderGpuScene>(renderHardwareInterface.GetResourceService(), gpuMeshCache)),
    m_renderRayTracingScene(std::make_unique<RenderRayTracingScene>(renderHardwareInterface, gpuMeshCache, rayTracingCapabilities)),
    m_gpuMeshCache(&gpuMeshCache),
    m_materialCache(std::make_unique<MaterialCache>(textureCache, renderHardwareInterface))
{
}

RenderScene::~RenderScene() noexcept = default;

const RenderSceneGpuBindings& RenderScene::UpdateGpuScene(
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    std::uint32_t frameIndex)
{
	return m_renderGpuScene->Update(preparedScene, view, frameIndex);
}

RenderRayTracingFrameBindings RenderScene::PrepareRayTracingFrame(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan& viewPlan) noexcept
{
	return m_renderRayTracingScene->Prepare(preparedScene, viewPlan);
}

bool RenderScene::IsRayTracingAvailable() const noexcept
{
	return m_renderRayTracingScene->IsAvailable();
}

bool RenderScene::Apply(const RenderSceneDelta& delta, RenderSceneDynamicData dynamic)
{
	if (!ValidateDelta(delta))
	{
		return false;
	}
	if (!ValidateDynamic(dynamic, delta))
	{
		return false;
	}

	ApplyValidatedDelta(delta);
	ApplyDynamic(std::move(dynamic));
	return true;
}

void RenderScene::BuildMaterials(PreparedRenderScene& preparedScene)
{
	m_materialCache->BuildMaterials(m_materials, m_materialRevision, preparedScene);
}
