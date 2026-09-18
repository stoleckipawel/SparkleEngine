#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Frame/RenderFrame.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/Presentation/Upscaling.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "View/RenderView.h"

ReferencePathTracer::ReferencePathTracer(
    RenderDeviceServices& deviceServices,
    RendererMemoryMonitor& memoryMonitor,
    RenderRayTracingScene& rayTracingScene) noexcept :
    m_rayTracingScene(rayTracingScene),
    m_session(deviceServices, memoryMonitor)
{
}

void ReferencePathTracer::AddPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources)
{
	m_session.ReserveGraphResources(builder, settings.RenderExtent);
	AddGpuPasses(builder, settings.RenderExtent, resources);
	const ReferencePathTracerGraphResources& graphResources = m_session.GetGraphResources();
	resources.ViewportProducts.RawSceneColor = graphResources.CommittedMean;
	resources.ViewportProducts.RawSceneColorMoment2 = graphResources.CommittedM2;
	AddExposurePass(builder, settings, resources);
	AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, nullptr, resources);
	resources.ViewportProducts.SceneDepth = FrameGraphTextureHandle::Invalid();
}

RenderProduct::Provenance ReferencePathTracer::GetRawProvenance() const noexcept
{
	return m_session.GetRawProvenance();
}

ViewportRenderProgress ReferencePathTracer::Update(
    const RenderFrame& frame,
    ViewportRenderAction action,
    std::uint64_t actionSequence) noexcept
{
	return m_session.Update(frame, action, actionSequence, m_rayTracingScene.GetExecutionFrontend());
}

bool ReferencePathTracer::BindResources(FrameGraph& frameGraph) const noexcept
{
	return m_session.BindResources(frameGraph);
}

void ReferencePathTracer::RecordSubmission(RhiSubmissionToken token) noexcept
{
	m_session.RecordSubmission(token);
}
