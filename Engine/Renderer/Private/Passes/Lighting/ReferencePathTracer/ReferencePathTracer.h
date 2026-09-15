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

class ReferencePathTracer final
{
public:
	ReferencePathTracer(RenderDeviceServices& deviceServices, RendererMemoryMonitor& memoryMonitor) noexcept;

	ReferencePathTracer(const ReferencePathTracer&) = delete;
	ReferencePathTracer& operator=(const ReferencePathTracer&) = delete;

	void AddPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);
	ViewportRenderProgress Update(
	    const RenderView& view,
	    const PreparedRenderScene& scene,
	    const RenderFrameIdentity& frame,
	    std::uint64_t sceneGeneration) noexcept;
	bool BindResources(FrameGraph& frameGraph) const noexcept;
	void RecordSubmission(RhiSubmissionToken token) noexcept;

private:
	ReferencePathTracerResources m_resources;
	ReferencePathTracerSession m_session;
};
