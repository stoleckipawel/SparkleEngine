#pragma once

#include "FrameGraph/FrameGraph.h"
#include "Pipeline/RenderPassRuntimeCache.h"

#include <cstdint>
#include <string_view>
#include <utility>

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

class FrameGraphBuilder final
{
public:
	FrameGraphBuilder(FrameGraph& frameGraph, const RenderPassRuntimeCache& renderPassRuntimeCache) noexcept;

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(std::string_view name, EFrameGraphPassKind kind, SetupFn&& setupFn, ExecuteFn&& executeFn)
	{
		AddPass(name, kind, EFrameGraphQueuePreference::Graphics, std::forward<SetupFn>(setupFn), std::forward<ExecuteFn>(executeFn));
	}

	template <typename SetupFn, typename ExecuteFn> void AddPass(
	    std::string_view name,
	    EFrameGraphPassKind kind,
	    EFrameGraphQueuePreference queuePreference,
	    SetupFn&& setupFn,
	    ExecuteFn&& executeFn)
	{
		m_frameGraph.AddPass(name, kind, queuePreference, std::forward<SetupFn>(setupFn), std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename... TDependencies>
	void Draw(typename TPass::ParameterInstance& parameters, TDependencies&... dependencies)
	{
		m_renderPassRuntimeCache.MaterializePassRuntime<TPass>();
		const TPass pass(m_renderPassRuntimeCache.GetPassRuntime<TPass>(), dependencies...);
		m_frameGraph.AddRasterPass<TPass>(
		    TPass::PassName,
		    parameters,
		    [pass](PassCommandContext& context, typename TPass::ParameterInstance& passParameters)
		    { pass.Execute(context, passParameters); });
	}

	template <typename TPass> void Dispatch(typename TPass::ParameterInstance& parameters)
	{
		m_renderPassRuntimeCache.MaterializePassRuntime<TPass>();
		const TPass pass(m_renderPassRuntimeCache.GetPassRuntime<TPass>());
		m_frameGraph.AddComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    [pass](PassCommandContext& context, typename TPass::ParameterInstance& passParameters)
		    { pass.Execute(context, passParameters); });
	}

	template <typename TPass>
	void Dispatch(typename TPass::ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight)
	{
		Dispatch<TPass>(TPass::PassName, parameters, outputWidth, outputHeight);
	}

	template <typename TPass> void DispatchAsync(typename TPass::ParameterInstance& parameters)
	{
		m_renderPassRuntimeCache.MaterializePassRuntime<TPass>();
		const TPass pass(m_renderPassRuntimeCache.GetPassRuntime<TPass>());
		m_frameGraph.AddAsyncComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    [pass](PassCommandContext& context, typename TPass::ParameterInstance& passParameters)
		    { pass.Execute(context, passParameters); });
	}

	template <typename TPass> void Dispatch(
	    std::string_view name,
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		m_renderPassRuntimeCache.MaterializePassRuntime<TPass>();
		const TPass pass(m_renderPassRuntimeCache.GetPassRuntime<TPass>());
		m_frameGraph.AddComputePass<TPass>(
		    name,
		    parameters,
		    [pass, outputWidth, outputHeight](PassCommandContext& context, typename TPass::ParameterInstance& passParameters)
		    { pass.Execute(context, passParameters, outputWidth, outputHeight); });
	}

	template <typename TPass>
	void DispatchAsync(typename TPass::ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight)
	{
		DispatchAsync<TPass>(TPass::PassName, parameters, outputWidth, outputHeight);
	}

	template <typename TPass> void DispatchAsync(
	    std::string_view name,
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		m_renderPassRuntimeCache.MaterializePassRuntime<TPass>();
		const TPass pass(m_renderPassRuntimeCache.GetPassRuntime<TPass>());
		m_frameGraph.AddAsyncComputePass<TPass>(
		    name,
		    parameters,
		    [pass, outputWidth, outputHeight](PassCommandContext& context, typename TPass::ParameterInstance& passParameters)
		    { pass.Execute(context, passParameters, outputWidth, outputHeight); });
	}

	template <typename TParameters> TypedPassParameterInstance<TParameters>& AllocParameters()
	{
		return m_frameGraph.AllocParameters<TParameters>();
	}

	template <typename TParameters, typename TCallback>
	void AddPassParameterSetup(TypedPassParameterInstance<TParameters>& parameters, TCallback&& callback)
	{
		auto* parameterInstance = &parameters;
		m_frameGraph.m_passParameterSetups.emplace_back(
		    [parameterInstance, setup = std::forward<TCallback>(callback)]() mutable
		    { setup(parameterInstance->GetFields()); });
	}

	template <typename TValue, typename TCallback> void AddParameterSetup(TCallback&& callback)
	{
		m_frameGraph.AddParameterSetup<TValue>(std::forward<TCallback>(callback));
	}

	template <typename TValue, typename TParameters, typename TCallback>
	void AddParameterSetup(TypedPassParameterInstance<TParameters>& parameters, TCallback&& callback)
	{
		auto* parameterInstance = &parameters;
		m_frameGraph.AddParameterSetup<TValue>(
		    [parameterInstance, setup = std::forward<TCallback>(callback)](const TValue& value) mutable
		    { setup(parameterInstance->GetFields(), value); });
	}

	template <typename TParameters, typename TCallback>
	void AddResourceProductionSetup(
	    TypedPassParameterInstance<TParameters>& parameters,
	    FrameGraphTextureHandle resource,
	    TCallback&& callback)
	{
		auto* parameterInstance = &parameters;
		auto* frameGraph = &m_frameGraph;
		m_frameGraph.m_resourceProductionSetups.emplace_back(
		    [parameterInstance, frameGraph, resource, setup = std::forward<TCallback>(callback)]() mutable
		    { setup(parameterInstance->GetFields(), frameGraph->HasBeenProduced(resource)); });
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
	    std::string_view name,
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
	FrameGraph& m_frameGraph;
	const RenderPassRuntimeCache& m_renderPassRuntimeCache;
};
