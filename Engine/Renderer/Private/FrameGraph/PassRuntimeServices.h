#pragma once

#include "Pipeline/PipelineStateManager.h"

#include "RHI/Public/Resources/PerFrameConstantBufferData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "Upscaling/RenderUpscalingPassServices.h"

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
	bool ExposureHistoryValid = false;
	const TextureManager* Textures = nullptr;
	const RenderRayTracingPassServices* RayTracing = nullptr;
	const RenderUpscalingPassServices* Upscaling = nullptr;

	template <typename TPass> const typename TPass::PipelineRuntime& GetPassRuntime() const noexcept
	{
		return RuntimeManager.GetPassRuntime<TPass>();
	}
};
