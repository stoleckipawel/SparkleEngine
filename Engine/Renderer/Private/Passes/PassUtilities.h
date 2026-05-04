#pragma once

#include "Frame/FrameContext.h"
#include "Renderer/Public/FrameGraph/BufferHandle.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "FrameGraph/RenderPassContext.h"
#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "GPU/CommandContext.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/ShaderPass.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace PassUtilities
{
	inline void DrawFullscreenTriangle(CommandContext& cmd) noexcept
	{
		cmd.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
		cmd.DrawInstanced(3, 1, 0, 0);
	}

	inline const PassParameterSet& GetEmptyPassParameterSet() noexcept
	{
		static const PassParameterLayout emptyLayout("PassUtilities.EmptyPassParameters");
		static const PassParameterSet emptyParameters(emptyLayout);
		return emptyParameters;
	}

	inline void AppendBindingNameIfCompiled(
	    std::vector<const char*>& bindingNames,
	    const RenderBindingLayout& bindingLayout,
	    const char* bindingName) noexcept
	{
		if (bindingName == nullptr || bindingName[0] == '\0' || bindingLayout.FindBinding(bindingName) == nullptr)
		{
			return;
		}

		const auto existing = std::ranges::find_if(
		    bindingNames,
		    [bindingName](const char* existingName)
		    {
			    return std::string_view(existingName != nullptr ? existingName : "") == bindingName;
		    });
		if (existing == bindingNames.end())
		{
			bindingNames.push_back(bindingName);
		}
	}

	inline std::vector<const char*> BuildBoundBindingNames(
	    const RenderBindingLayout& bindingLayout,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides) noexcept
	{
		std::vector<const char*> bindingNames;
		if (const PassParameterLayout* parameterLayout = parameters.GetLayout())
		{
			const std::vector<PassParameterDesc>& parameterDescs = parameterLayout->GetParameters();
			for (std::size_t index = 0; index < parameterDescs.size() && index < parameters.GetBindingCount(); ++index)
			{
				const PassParameterBinding* binding = parameters.GetBinding(static_cast<std::uint32_t>(index));
				if (binding != nullptr && binding->IsBound())
				{
					AppendBindingNameIfCompiled(bindingNames, bindingLayout, parameterDescs[index].Name.c_str());
				}
			}
		}

		if (overrides != nullptr)
		{
			for (const PassBindingOverride& bindingOverride : overrides->GetOverrides())
			{
				AppendBindingNameIfCompiled(bindingNames, bindingLayout, bindingOverride.Name.c_str());
			}
		}

		return bindingNames;
	}

	template <typename TRasterPassRuntime>
	bool BindRasterPassWithRuntime(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPassRuntime& runtime,
	    const PassParameterSet& parameters,
	    const char* const* bindingNames = nullptr,
	    std::uint32_t bindingNameCount = 0,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		return RasterShaderPass<PassParameterSet>::Bind(
		    frameGraph,
		    cmd,
		    renderHardwareInterface,
		    runtime.BindingLayout,
		    runtime.PipelineState,
		    parameters,
		    bindingNames,
		    bindingNameCount,
		    overrides,
		    passName);
	}

	template <typename TRasterPassRuntime>
	bool BindAvailableRasterPassWithRuntime(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPassRuntime& runtime,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		const std::vector<const char*> bindingNames = BuildBoundBindingNames(runtime.BindingLayout, parameters, overrides);
		return BindRasterPassWithRuntime(
		    frameGraph,
		    cmd,
		    renderHardwareInterface,
		    runtime,
		    parameters,
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    overrides,
		    passName);
	}

	template <typename TRasterPassRuntime, std::size_t N>
	bool BindRasterPassOverridesWithRuntime(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPassRuntime& runtime,
	    const std::array<const char*, N>& bindingNames,
	    const PassBindingOverrides& overrides,
	    const char* passName = nullptr) noexcept
	{
		return BindRasterPassWithRuntime(
		    frameGraph,
		    cmd,
		    renderHardwareInterface,
		    runtime,
		    GetEmptyPassParameterSet(),
		    bindingNames.data(),
		    static_cast<std::uint32_t>(bindingNames.size()),
		    &overrides,
		    passName);
	}

	template <typename TRasterPassRuntime>
	bool BindRasterPassOverridesWithRuntime(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface* renderHardwareInterface,
	    const TRasterPassRuntime& runtime,
	    const PassBindingOverrides& overrides,
	    const char* passName = nullptr) noexcept
	{
		return BindAvailableRasterPassWithRuntime(
		    frameGraph,
		    cmd,
		    renderHardwareInterface,
		    runtime,
		    GetEmptyPassParameterSet(),
		    &overrides,
		    passName);
	}

	template <typename TComputePass, typename TComputePassRuntime, typename TParameterBindings>
	bool DispatchComputePassWithRuntime(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const char* passName) noexcept
	{
		using Parameters = typename TComputePass::Parameters;

		return ComputeShaderPass<Parameters>::Dispatch(
		    frameGraph,
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
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
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
		    frameGraph,
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
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const PassParameterSet& parameters,
	    const ComputeDispatchDesc& dispatch,
	    const PassBindingOverrides* overrides = nullptr,
	    const char* passName = nullptr) noexcept
	{
		const std::vector<const char*> bindingNames = BuildBoundBindingNames(runtime.BindingLayout, parameters, overrides);
		return DispatchComputePassWithRuntime<TComputePass>(
		    frameGraph,
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

	inline void AddCopyTexturePass(
	    FrameGraph& frameGraph,
	    std::string_view name,
	    TextureHandle destinationHandle,
	    TextureHandle sourceHandle)
	{
		frameGraph.AddPass(
		    name,
		    EFrameGraphPassFlags::Transfer,
		    [destinationHandle, sourceHandle](PassBuilder& builder)
		    {
			    builder.Read(sourceHandle, ResourceUsage::CopySource);
			    builder.Write(destinationHandle, ResourceUsage::CopyDest);
		    },
		    [destinationHandle, sourceHandle](RenderGraphPassContext& context)
		    {
			    context.Graph.CopyTexture(context.Commands, destinationHandle, sourceHandle);
		    });
	}

	inline void AddCopyBufferPass(FrameGraph& frameGraph, std::string_view name, BufferHandle destinationHandle, BufferHandle sourceHandle)
	{
		frameGraph.AddPass(
		    name,
		    EFrameGraphPassFlags::Transfer,
		    [destinationHandle, sourceHandle](PassBuilder& builder)
		    {
			    builder.Read(sourceHandle, ResourceUsage::CopySource);
			    builder.Write(destinationHandle, ResourceUsage::CopyDest);
		    },
		    [destinationHandle, sourceHandle](RenderGraphPassContext& context)
		    {
			    context.Graph.CopyBuffer(context.Commands, destinationHandle, sourceHandle);
		    });
	}
}  // namespace PassUtilities