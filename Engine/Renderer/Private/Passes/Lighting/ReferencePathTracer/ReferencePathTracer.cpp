#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"
#include "Passes/PostProcessing/Exposure.h"
#include "Passes/Presentation/Upscaling.h"

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
    const RenderFrameTime& time,
    std::uint64_t sceneGeneration) noexcept
{
	return m_session.Update(active, view, scene, frame, time, sceneGeneration, m_resources);
}

bool ReferencePathTracer::BindResources(FrameGraph& frameGraph) const noexcept
{
	return !m_session.IsSelected() || m_resources.Bind(frameGraph);
}

void ReferencePathTracer::RecordSubmission(RhiSubmissionToken token) noexcept
{
	m_session.RecordSubmission(token, m_resources);
}

void ReferencePathTracer::SetTargetSampleCount(std::uint32_t target) noexcept
{
	m_session.SetTargetSampleCount(target);
}

void ReferencePathTracer::Pause() noexcept
{
	m_session.Pause();
}

void ReferencePathTracer::Resume() noexcept
{
	m_session.Resume();
}

void ReferencePathTracer::Restart() noexcept
{
	m_session.Restart(m_resources);
}

void ReferencePathTracer::Cancel() noexcept
{
	m_session.Cancel(m_resources);
}

ReferencePathTracerProgress ReferencePathTracer::GetProgress() const noexcept
{
	return m_session.GetProgress();
}
