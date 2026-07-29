#pragma once

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/Core/ShaderPass.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <array>
#include <type_traits>
#include <utility>
#include <vector>

namespace ShaderPassOperations
{
	void DrawFullscreenTriangle(RenderCommandContext& commandContext) noexcept;
	const PassParameterSet& GetEmptyPassParameterSet() noexcept;
	void AppendBindingNameIfCompiled(
	    std::vector<const char*>& bindingNames,
	    const RenderBindingLayout& bindingLayout,
	    const char* bindingName) noexcept;
	std::vector<const char*> BuildBoundBindingNames(
	    const RenderBindingLayout& bindingLayout,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides) noexcept;

	template <typename TRasterPipelineRuntime>
	const RenderPipeline& ResolveRasterPipeline(
	    const TRasterPipelineRuntime& runtime,
	    std::uint32_t viewModeIndex) noexcept
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

	template <typename TRasterPipelineRuntime>
	bool BindRasterPassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPipelineRuntime& runtime,
	    const PassParameterSet& parameters,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr,
	    bool bindLayout = true,
	    std::uint32_t viewModeIndex = 0u) noexcept
	{
		return RasterShaderPass<PassParameterSet>::Bind(
		    resources,
		    commandContext,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    ResolveRasterPipeline(runtime, viewModeIndex),
		    parameters,
		    bindingNames,
		    bindingNameCount,
		    overrides,
		    passName,
		    bindLayout);
	}

	template <typename TRasterPipelineRuntime>
	bool BindAvailableRasterPassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface* renderHardwareInterface,
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
		    renderHardwareInterface,
		    runtime,
		    parameters,
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    overrides,
		    passName,
		    bindLayout,
		    viewModeIndex);
	}

	template <typename TRasterPipelineRuntime, std::size_t N>
	bool BindRasterPassOverridesWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPipelineRuntime& runtime,
	    const std::array<const char*, N>& bindingNames,
	    const PassBindingOverrides& overrides,
	    const char* passName = nullptr) noexcept
	{
		return BindRasterPassWithRuntime(
		    resources,
		    commandContext,
		    renderHardwareInterface,
		    runtime,
		    GetEmptyPassParameterSet(),
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    &overrides,
		    passName);
	}

	template <typename TRasterPipelineRuntime>
	bool BindRasterPassOverridesWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPipelineRuntime& runtime,
	    const PassBindingOverrides& overrides,
	    const char* passName = nullptr) noexcept
	{
		return BindAvailableRasterPassWithRuntime(
		    resources,
		    commandContext,
		    renderHardwareInterface,
		    runtime,
		    GetEmptyPassParameterSet(),
		    &overrides,
		    passName);
	}

	template <typename TComputePass, typename TComputePassRuntime, typename TParameterBindings>
	bool DispatchComputePassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const char* passName) noexcept
	{
		using Parameters = typename TComputePass::Parameters;

		return ComputeShaderPass<Parameters>::Dispatch(
		    resources,
		    commandContext,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    runtime.Pipeline,
		    parameters,
		    dispatch,
		    nullptr,
		    0,
		    nullptr,
		    passName);
	}

	template <typename TComputePass, typename TComputePassRuntime, typename TParameterBindings>
	bool DispatchComputePassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		using Parameters = typename TComputePass::Parameters;

		return ComputeShaderPass<Parameters>::Dispatch(
		    resources,
		    commandContext,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    runtime.Pipeline,
		    parameters,
		    dispatch,
		    bindingNames,
		    bindingNameCount,
		    overrides,
		    passName);
	}

	template <typename TComputePass, typename TComputePassRuntime>
	bool DispatchAvailableComputePassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const PassParameterSet& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		const std::vector<const char*> bindingNames = BuildBoundBindingNames(runtime.BindingLayout, parameters, overrides);
		return DispatchComputePassWithRuntime<TComputePass>(
		    resources,
		    commandContext,
		    renderHardwareInterface,
		    runtime,
		    parameters,
		    dispatch,
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    overrides,
		    passName);
	}

}  // namespace ShaderPassOperations
