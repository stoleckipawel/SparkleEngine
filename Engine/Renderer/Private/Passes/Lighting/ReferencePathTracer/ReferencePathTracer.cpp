#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/Presentation/Upscaling.h"
#include "RayTracing/Effects/RayTracingExecutionFrontend.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "View/RenderView.h"

ReferencePathTracer::ReferencePathTracer(RenderDeviceServices& deviceServices, RendererMemoryMonitor& memoryMonitor) noexcept :
    m_deviceServices(deviceServices),
    m_resources(deviceServices, memoryMonitor),
    m_session(deviceServices)
{
}

void ReferencePathTracer::AddPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources,
    RenderRayTracingScene& rayTracingScene)
{
	m_resources.ReserveGraphResources(builder, settings.RenderExtent);
	const RayTracingExecutionFrontend executionFrontend =
	    ResolveRayTracingExecutionFrontend(rayTracingScene.GetCapabilityReport());
	AddReferencePathTracerGpuPasses(
	    builder,
	    settings.RenderExtent,
	    resources,
	    m_resources.GetGraphResources(),
	    m_session.GetUniformData(),
	    ReferencePathTracerSession::WorkRowsPerDispatch,
	    executionFrontend,
	    rayTracingScene.GetShaderTablePlan());
	AddExposurePass(builder, settings, resources);
	AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, nullptr, resources);
	resources.ViewportProducts.SceneDepth = FrameGraphTextureHandle::Invalid();
}

ViewportRenderProgress ReferencePathTracer::Update(
    const ViewportRenderRequest& request,
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration) noexcept
{
	return m_session.Update(
	    request,
	    view,
	    scene,
	    frame,
	    sceneGeneration,
	    ResolveRayTracingExecutionFrontend(BuildRayTracingCapabilityReport(m_deviceServices.GetCapabilities())),
	    m_resources);
}

bool ReferencePathTracer::BindResources(FrameGraph& frameGraph) const noexcept
{
	return !m_session.IsSelected() || (m_session.CanBindResources() && m_resources.Bind(frameGraph));
}

void ReferencePathTracer::RecordSubmission(RhiSubmissionToken token) noexcept
{
	m_session.RecordSubmission(token, m_resources);
}
