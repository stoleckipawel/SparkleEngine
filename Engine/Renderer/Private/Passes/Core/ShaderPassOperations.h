#pragma once

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/Core/ShaderPass.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <cstdint>
#include <vector>

namespace ShaderPassOperations
{
	std::vector<const char*> BuildBoundBindingNames(
	    const RenderBindingLayout& bindingLayout,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides) noexcept;

	template <typename TRasterPipelineRuntime> bool BindRasterPassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const TRasterPipelineRuntime& runtime,
	    const PassParameterSet& parameters,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr,
	    bool bindLayout = true) noexcept
	{
		return BindRasterShader(
		    resources,
		    commandContext,
		    runtime.BindingLayout,
		    runtime.Pipeline,
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
	    bool bindLayout = true) noexcept
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
		    bindLayout);
	}

}
