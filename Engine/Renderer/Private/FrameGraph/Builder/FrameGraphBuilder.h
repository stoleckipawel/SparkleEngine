#pragma once

#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"

#include <cstdint>
#include <string_view>
#include <utility>

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

class PipelineStateManager;

class FrameGraphBuilder final
{
  public:
	FrameGraphBuilder(
	    FrameGraph& frameGraph,
	    const PipelineStateManager& pipelineStateManager) noexcept;

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(std::string_view name, EFrameGraphPassKind kind, SetupFn&& setupFn, ExecuteFn&& executeFn)
	{
		AddPass(
		    name,
		    kind,
		    EFrameGraphQueuePreference::Graphics,
		    std::forward<SetupFn>(setupFn),
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(
	    std::string_view name,
	    EFrameGraphPassKind kind,
	    EFrameGraphQueuePreference queuePreference,
	    SetupFn&& setupFn,
	    ExecuteFn&& executeFn)
	{
		m_frameGraph.AddPass(
		    name,
		    kind,
		    queuePreference,
		    std::forward<SetupFn>(setupFn),
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass> void Draw(typename TPass::ParameterInstance& parameters)
	{
		m_pipelineStateManager.MaterializePassRuntime<TPass>();
		m_frameGraph.AddRasterPass<TPass>(
		    TPass::PassName,
		    parameters,
		    &ExecutePass<TPass>);
	}

	template <typename TPass> void Dispatch(typename TPass::ParameterInstance& parameters)
	{
		m_pipelineStateManager.MaterializePassRuntime<TPass>();
		m_frameGraph.AddComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    &ExecutePass<TPass>);
	}

	template <typename TPass>
	void Dispatch(
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		Dispatch<TPass>(TPass::PassName, parameters, outputWidth, outputHeight);
	}

	template <typename TPass> void DispatchAsync(typename TPass::ParameterInstance& parameters)
	{
		m_pipelineStateManager.MaterializePassRuntime<TPass>();
		m_frameGraph.AddAsyncComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    &ExecutePass<TPass>);
	}

	template <typename TPass>
	void Dispatch(
	    std::string_view name,
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		m_pipelineStateManager.MaterializePassRuntime<TPass>();
		m_frameGraph.AddComputePass<TPass>(
		    name,
		    parameters,
		    [outputWidth, outputHeight](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    ExecuteSizedPass<TPass>(context, passParameters, outputWidth, outputHeight);
		    });
	}

	template <typename TPass>
	void DispatchAsync(
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		DispatchAsync<TPass>(TPass::PassName, parameters, outputWidth, outputHeight);
	}

	template <typename TPass>
	void DispatchAsync(
	    std::string_view name,
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		m_pipelineStateManager.MaterializePassRuntime<TPass>();
		m_frameGraph.AddAsyncComputePass<TPass>(
		    name,
		    parameters,
		    [outputWidth, outputHeight](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    ExecuteSizedPass<TPass>(context, passParameters, outputWidth, outputHeight);
		    });
	}

	template <typename TPass, typename ConditionFn>
	void DispatchIf(
	    typename TPass::ParameterInstance& parameters,
	    ConditionFn&& condition)
	{
		const PipelineStateManager* const pipelineStateManager =
		    &m_pipelineStateManager;

		m_frameGraph.AddConditionalComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    [pipelineStateManager,
		     condition = std::forward<ConditionFn>(condition)](
		        const FrameContext& frame) mutable
		    {
			    if (!condition(frame))
			    {
				    return false;
			    }

			    pipelineStateManager->MaterializePassRuntime<TPass>();
			    return true;
		    },
		    &ExecutePass<TPass>);
	}

	template <typename TParameters> TypedPassParameterInstance<TParameters>& AllocParameters()
	{
		return m_frameGraph.AllocParameters<TParameters>();
	}

	FrameGraphTextureHandle ImportBackBuffer(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	FrameGraphTextureHandle ReservePersistentTexture(
	    const FrameGraphTextureDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphTextureHandle CreateTexture(const FrameGraphTextureDesc& desc) noexcept;
	FrameGraphTextureHistory CreateTextureHistory(const FrameGraphTextureDesc& desc) noexcept;
	FrameGraphBufferHandle ReservePersistentBuffer(
	    const FrameGraphBufferDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphBufferHandle CreateBuffer(const FrameGraphBufferDesc& desc) noexcept;
	FrameGraphAccelerationStructureHandle ReservePersistentAccelerationStructure(
	    const FrameGraphAccelerationStructureDesc& desc,
	    ResourceState initialState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void ExportTexture(FrameGraphTextureHandle handle, std::string_view name) noexcept;

	template <typename TValue = void> ShaderTexture2D<TValue> Read(FrameGraphTextureHandle handle) const noexcept
	{
		return m_frameGraph.Read<TValue>(handle);
	}

	template <typename TValue = void> ShaderTexture2D<TValue> CreateSRV(FrameGraphTextureHandle handle) const noexcept
	{
		return m_frameGraph.CreateSRV<TValue>(handle);
	}

	template <typename TValue = void> ShaderBuffer<TValue> Read(FrameGraphBufferHandle handle) const noexcept
	{
		return m_frameGraph.Read<TValue>(handle);
	}

	template <typename TValue = void> ShaderBuffer<TValue> CreateSRV(FrameGraphBufferHandle handle) const noexcept
	{
		return m_frameGraph.CreateSRV<TValue>(handle);
	}

	template <typename TValue = void> ShaderRWTexture2D<TValue> CreateUAV(FrameGraphTextureHandle handle) const noexcept
	{
		return m_frameGraph.CreateUAV<TValue>(handle);
	}

	template <typename TValue = void> ShaderRWBuffer<TValue> CreateUAV(FrameGraphBufferHandle handle) const noexcept
	{
		return m_frameGraph.CreateUAV<TValue>(handle);
	}

	ShaderAccelerationStructure Read(FrameGraphAccelerationStructureHandle handle) const noexcept;
	ShaderRenderTarget CreateRenderTarget(FrameGraphTextureHandle handle) const noexcept;
	ShaderDepthTarget CreateDepthTarget(FrameGraphTextureHandle handle) const noexcept;

  private:
	template <typename TPass>
	static void ExecutePass(
	    PassExecutionContext& context,
	    typename TPass::ParameterInstance& parameters)
	{
		const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
		pass.Execute(context, parameters);
	}

	template <typename TPass>
	static void ExecuteSizedPass(
	    PassExecutionContext& context,
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
		pass.Execute(context, parameters, outputWidth, outputHeight);
	}

	FrameGraph& m_frameGraph;
	const PipelineStateManager& m_pipelineStateManager;
};
