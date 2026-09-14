#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerCVar.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/Presentation/Upscaling.h"

ConsoleVariable<bool> CVarReferencePathTracer("r.ReferencePathTracer", false, "Use the Reference Path Tracer frame composition.");

ReferencePathTracer::ReferencePathTracer(RenderDeviceServices& deviceServices, RendererMemoryMonitor& memoryMonitor) noexcept :
    m_resources(deviceServices, memoryMonitor),
    m_session(deviceServices)
{
}

void ReferencePathTracer::AddPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources)
{
	m_resources.ReserveGraphResources(builder, settings.RenderExtent);
	AddReferencePathTracerGpuPasses(
	    builder,
	    settings.RenderExtent,
	    resources,
	    m_resources.GetGraphResources(),
	    m_session.GetUniformData(),
	    ReferencePathTracerSession::WorkRowsPerDispatch);
	AddExposurePass(builder, settings, resources);
	AddUpscalingPasses(builder, settings.RenderExtent, settings.OutputExtent, nullptr, resources);
	resources.ViewportProducts.SceneDepth = FrameGraphTextureHandle::Invalid();
}

ViewportRenderProgress ReferencePathTracer::Update(
    bool active,
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration) noexcept
{
	return m_session.Update(active, view, scene, frame, sceneGeneration, m_resources);
}

bool ReferencePathTracer::BindResources(FrameGraph& frameGraph) const noexcept
{
	return !m_session.IsSelected() || m_resources.Bind(frameGraph);
}

void ReferencePathTracer::RecordSubmission(RhiSubmissionToken token) noexcept
{
	m_session.RecordSubmission(token, m_resources);
}
