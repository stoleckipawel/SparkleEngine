#pragma once

#include "ReferencePathTracerSession.h"

#include <cstdint>

class FrameGraph;
class FrameGraphBuilder;
class FramePipeline;
class RendererMemoryMonitor;
class RenderDeviceServices;
struct RenderFrame;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;
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

private:
	friend class FramePipeline;

	void AddPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);
	void AddGpuPasses(FrameGraphBuilder& builder, RenderViewportExtent extent, const RenderFrameGraphResources& resources);
	ViewportRenderProgress Update(const RenderFrame& frame, ViewportRenderAction action, std::uint64_t actionSequence) noexcept;
	bool BindResources(FrameGraph& frameGraph) const noexcept;
	void RecordSubmission(RhiSubmissionToken token) noexcept;
	RenderRayTracingScene& m_rayTracingScene;
	ReferencePathTracerSession m_session;
};
