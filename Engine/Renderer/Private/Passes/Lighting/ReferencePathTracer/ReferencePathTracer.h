#pragma once

#include "ReferencePathTracerResources.h"
#include "ReferencePathTracerSession.h"

#include <cstdint>

class FrameGraph;
class FrameGraphBuilder;
class RendererMemoryMonitor;
class RenderDeviceServices;
struct PreparedRenderScene;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;
struct RenderFrameIdentity;
struct RenderView;
class RenderRayTracingScene;

class ReferencePathTracer final
{
public:
	ReferencePathTracer(
	    RenderDeviceServices& deviceServices,
	    RendererMemoryMonitor& memoryMonitor,
	    RenderRayTracingScene& rayTracingScene) noexcept;

	ReferencePathTracer(const ReferencePathTracer&) = delete;
	ReferencePathTracer& operator=(const ReferencePathTracer&) = delete;

	void AddPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);
	ViewportRenderProgress Update(
	    const ViewportRenderRequest& request,
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration) noexcept;
	bool BindResources(FrameGraph& frameGraph) const noexcept;
	void RecordSubmission(RhiSubmissionToken token) noexcept;

private:
	RenderRayTracingScene& m_rayTracingScene;
	ReferencePathTracerResources m_resources;
	ReferencePathTracerSession m_session;
};
