#pragma once

#include "Pipeline/PipelineStateManager.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RayTracing/RenderRayTracingPassServices.h"

class TextureManager;

// Stable execute-time runtime services exposed to authored render passes.
//
// This surface owns the renderer services that remain intentionally global at
// pass execution time, plus typed runtime lookup by pass type.
//
// Keep this struct focused. New fields should only be added when they are
// genuinely stable execute-time services shared across authored passes.
struct PassRuntimeServices
{
	RenderHardwareInterface& HardwareInterface;
	RenderDiagnostics& BackendDiagnostics;
	const PipelineStateManager& RuntimeManager;
	const TextureManager* Textures = nullptr;
	const RenderRayTracingPassServices* RayTracing = nullptr;

	template <typename TPass> const typename RenderPassPipelineTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		return RuntimeManager.GetPassRuntime<TPass>();
	}
};
