#pragma once

#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "Frame/Presentation/ViewportDisplaySettings.h"

#include "ShaderData/FrameUniformData.h"
#include "RayTracing/Scene/RayTracingPassContext.h"

class RenderHardwareInterface;
class GpuMeshCache;
class TextureCache;

// Per-frame runtime context exposed to FrameGraph passes.
//
// This surface references persistent renderer caches without owning them and
// carries the transient frame state shared across authored passes.
//
// Keep this struct focused. New fields should only be added when they are
// genuinely shared across pass execution.
struct PassRuntimeContext
{
	RenderHardwareInterface& HardwareInterface;
	const RenderPassRuntimeCache& PassRuntimes;
	const FrameUniformData& Frame;
	const ResolvedViewportDisplaySettings& DisplaySettings;
	FrameHistoryValidity History = {};
	const GpuMeshCache* Meshes = nullptr;
	const TextureCache* Textures = nullptr;
	const RayTracingPassContext* RayTracing = nullptr;
	const ImageProviderPassContext* ImageProviders = nullptr;

	template <typename TPass> const typename TPass::PipelineRuntime& GetPassRuntime() const noexcept
	{
		return PassRuntimes.GetPassRuntime<TPass>();
	}
};
