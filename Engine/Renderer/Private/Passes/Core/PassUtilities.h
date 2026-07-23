#pragma once

#include "Frame/Core/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Commands/RenderCommandContext.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/Core/ShaderPass.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace PassUtilities
{
	void DrawFullscreenTriangle(RenderCommandContext& cmd) noexcept;
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
	const RenderPipelineState& ResolveRasterPipelineState(
	    const TRasterPipelineRuntime& runtime,
	    std::uint32_t viewModeIndex) noexcept
	{
		if constexpr (requires { runtime.WireframePipelineState; })
		{
			if (runtime.WireframePipelineState != nullptr && viewModeIndex == static_cast<std::uint32_t>(RenderViewMode::Wireframe))
			{
				return *runtime.WireframePipelineState;
			}
		}

		return runtime.PipelineState;
	}

	template <typename TRasterPipelineRuntime>
	bool BindRasterPassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
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
		    cmd,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    ResolveRasterPipelineState(runtime, viewModeIndex),
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
	    RenderCommandContext& cmd,
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
		    cmd,
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
	    RenderCommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPipelineRuntime& runtime,
	    const std::array<const char*, N>& bindingNames,
	    const PassBindingOverrides& overrides,
	    const char* passName = nullptr) noexcept
	{
		return BindRasterPassWithRuntime(
		    resources,
		    cmd,
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
	    RenderCommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPipelineRuntime& runtime,
	    const PassBindingOverrides& overrides,
	    const char* passName = nullptr) noexcept
	{
		return BindAvailableRasterPassWithRuntime(
		    resources,
		    cmd,
		    renderHardwareInterface,
		    runtime,
		    GetEmptyPassParameterSet(),
		    &overrides,
		    passName);
	}

	template <typename TComputePass, typename TComputePassRuntime, typename TParameterBindings>
	bool DispatchComputePassWithRuntime(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const char* passName) noexcept
	{
		using Parameters = typename TComputePass::Parameters;

		return ComputeShaderPass<Parameters>::Dispatch(
		    resources,
		    cmd,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    runtime.PipelineState,
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
	    RenderCommandContext& cmd,
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
		    cmd,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    runtime.PipelineState,
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
	    RenderCommandContext& cmd,
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
		    cmd,
		    renderHardwareInterface,
		    runtime,
		    parameters,
		    dispatch,
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    overrides,
		    passName);
	}

	void AddCopyTexturePass(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphTextureHandle destinationHandle,
	    FrameGraphTextureHandle sourceHandle);
	void AddCopyBufferPass(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphBufferHandle destinationHandle,
	    FrameGraphBufferHandle sourceHandle);
}  // namespace PassUtilities
