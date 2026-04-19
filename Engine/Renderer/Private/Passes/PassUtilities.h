#pragma once

#include "Frame/FrameContext.h"
#include "Renderer/Public/FrameGraph/BufferHandle.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "FrameGraph/RenderPassContext.h"
#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "GPU/CommandContext.h"
#include "Passes/ShaderPass.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <array>
#include <string_view>
#include <type_traits>
#include <utility>

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

	template <typename TComputePass, typename TComputePassRuntime, typename TParameterBindings>
	bool DispatchComputePassWithRuntime(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    RenderHardwareInterface& renderHardwareInterface,
	    const TComputePassRuntime& runtime,
	    const TParameterBindings& parameters,
	    const ComputeDispatchDesc& dispatch,
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