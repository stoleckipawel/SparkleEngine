#pragma once

#include "Pipeline/PipelineStateManager.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"

#include "ShaderData/PerFrameConstantBufferData.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"

class RenderHardwareInterface;
class GPUMeshCache;
class TextureManager;

// Stable execute-time runtime services exposed to FrameGraph passes.
//
// This surface owns the renderer services that remain intentionally global at
// pass execution time, plus typed runtime lookup for authored shader passes.
//
// Keep this struct focused. New fields should only be added when they are
// genuinely stable execute-time services shared across authored passes.
struct PassRuntimeServices
{
	RenderHardwareInterface& HardwareInterface;
	const PipelineStateManager& RuntimeManager;
	const PerFrameConstantBufferData& PerFrame;
	FrameHistoryValidity History = {};
	const GPUMeshCache* Meshes = nullptr;
	const TextureManager* Textures = nullptr;
	const RenderRayTracingPassServices* RayTracing = nullptr;
	const RendererImageProviderPassServices* ImageProviders = nullptr;

	template <typename TPass> const typename TPass::PipelineRuntime& GetPassRuntime() const noexcept
	{
		return RuntimeManager.GetPassRuntime<TPass>();
	}
};
