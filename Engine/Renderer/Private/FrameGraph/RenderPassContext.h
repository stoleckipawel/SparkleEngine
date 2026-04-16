#pragma once

#include "RendererAPI.h"
#include "FrameGraph/RenderPassRuntime.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"

// Stable execute-time runtime services exposed to authored render passes.
//
// This surface owns the renderer services that remain intentionally global at
// pass execution time, plus typed runtime lookup by pass type.
//
// Keep this struct focused. New fields should only be added when they are
// genuinely stable execute-time services shared across authored passes.
struct SPARKLE_RENDERER_API RenderPassContext
{
	RenderHardwareInterface& HardwareInterface;
	RhiDescriptorTableHandle SamplerTableHandle;
	const RenderPassRuntimeRegistry& RuntimeRegistry;

	template <typename TPass> const typename RenderPassRuntimeTraits<TPass>::RuntimeType& GetPassRuntime() const noexcept
	{
		return RuntimeRegistry.GetPassRuntime<TPass>();
	}
};
