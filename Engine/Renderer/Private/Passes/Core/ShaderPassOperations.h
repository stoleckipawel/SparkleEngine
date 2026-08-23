#pragma once

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/Core/ShaderPass.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <vector>

namespace ShaderPassOperations
{
	std::vector<const char*> BuildBoundBindingNames(
	    const RenderBindingLayout& bindingLayout,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides) noexcept;

	template <typename TRasterPipelineRuntime>
	const RenderPipeline& ResolveRasterPipeline(const TRasterPipelineRuntime& runtime, std::uint32_t viewModeIndex) noexcept
	{
		if constexpr (requires { runtime.WireframePipeline; })
		{
			if (runtime.WireframePipeline != nullptr && viewModeIndex == static_cast<std::uint32_t>(RenderViewMode::Wireframe))
			{
				return *runtime.WireframePipeline;
			}
		}

		return runtime.Pipeline;
	}

	template <typename TRasterPipelineRuntime> bool BindRasterPassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const TRasterPipelineRuntime& runtime,
	    const PassParameterSet& parameters,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr,
	    bool bindLayout = true,
	    std::uint32_t viewModeIndex = 0u) noexcept
	{
		return BindRasterShader(
		    resources,
		    commandContext,
		    runtime.BindingLayout,
		    ResolveRasterPipeline(runtime, viewModeIndex),
		    parameters,
		    bindingNames,
		    bindingNameCount,
		    overrides,
		    passName,
		    bindLayout);
	}

	template <typename TRasterPipelineRuntime> bool BindAvailableRasterPassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const TRasterPipelineRuntime& runtime,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr,
	    bool bindLayout = true,
	    std::uint32_t viewModeIndex = 0u) noexcept
	{
		const std::vector<const char*> bindingNames = BuildBoundBindingNames(runtime.BindingLayout, parameters, overrides);
		return BindRasterPassWithRuntime(
		    resources,
		    commandContext,
		    runtime,
		    parameters,
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    overrides,
		    passName,
		    bindLayout,
		    viewModeIndex);
	}

} // namespace ShaderPassOperations
