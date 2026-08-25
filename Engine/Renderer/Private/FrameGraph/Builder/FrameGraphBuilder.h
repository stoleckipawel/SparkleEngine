#pragma once

#include "FrameGraph/FrameGraph.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Pipeline/RasterPassRenderState.h"

#include <algorithm>
#include <cstdint>
#include <cassert>
#include <string>
#include <string_view>
#include <utility>

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Shaders/ShaderParameterLayoutBuilder.h"

class FrameGraphBuilder final
{
public:
	FrameGraphBuilder(FrameGraph& frameGraph, const RenderPassRuntimeCache& renderPassRuntimeCache) noexcept;

	template <typename TVertexShader, typename TPixelShader, typename TParameters, typename TDrawCollaborator> void Draw(
	    TypedPassParameterInstance<TParameters>& parameters,
	    const RasterPassRenderState& renderState,
	    TDrawCollaborator drawCollaborator)
	{
		const ShaderRegistrationDesc& shader = GlobalShader<TPixelShader>::GetRegistration();
		Draw<TVertexShader, TPixelShader>(shader.ShaderName, parameters, renderState, std::move(drawCollaborator));
	}

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

	template <typename TVertexShader, typename TPixelShader, typename TParameters, typename TDrawCollaborator> void Draw(
	    std::string_view label,
	    TypedPassParameterInstance<TParameters>& parameters,
	    const RasterPassRenderState& renderState,
	    TDrawCollaborator drawCollaborator)
	{
		auto rasterPass = std::make_shared<FrameGraphRasterPass>();
		auto* parameterInstance = &parameters;
		auto* frameGraph = &m_frameGraph;
		auto* runtimeCache = &m_renderPassRuntimeCache;
		const FrameGraphPassIndex passIndex = static_cast<FrameGraphPassIndex>(m_frameGraph.m_passes.size());
		m_frameGraph.m_passPreparations.emplace_back(
		    passIndex,
		    [parameterInstance, frameGraph, runtimeCache, renderState, rasterPass, materializer = drawCollaborator]() mutable
		    {
			    *rasterPass = frameGraph->BuildRasterPass(parameterInstance->GetPassParameterSet(), renderState);
			    materializer.MaterializePipelines(*runtimeCache, renderState, rasterPass->Compatibility);
		    });
		const std::string diagnosticLabel(label);
		m_frameGraph.AddRasterPass(
		    diagnosticLabel,
		    parameters,
		    [rasterPass, drawCollaborator = std::move(drawCollaborator)](
		        PassCommandContext& context,
		        TypedPassParameterInstance<TParameters>& passParameters) mutable
		    {
			    drawCollaborator.PrepareRasterPass(context.Commands);
			    context.Resources.BeginRasterPass(context.Commands, *rasterPass);
			    drawCollaborator.Draw(context, passParameters);
			    context.Resources.EndRasterPass(context.Commands);
		    });
	}

	template <typename TShader>
	void Dispatch(TypedPassParameterInstance<typename TShader::Parameters>& parameters, const ComputeDispatchDesc& groupCount)
	{
		const ShaderRegistrationDesc& shader = GlobalShader<TShader>::GetRegistration();
		Dispatch<TShader>(shader.ShaderName, parameters, groupCount);
	}

	template <typename TShader> void Dispatch(
	    std::string_view label,
	    TypedPassParameterInstance<typename TShader::Parameters>& parameters,
	    const ComputeDispatchDesc& groupCount)
	{
		m_renderPassRuntimeCache.MaterializeComputeShaderRuntime<TShader>();
		const ComputePassPipelineRuntime* const runtime = &m_renderPassRuntimeCache.GetComputeShaderRuntime<TShader>();
		const std::string diagnosticLabel(label);
		m_frameGraph.AddComputePass(
		    diagnosticLabel,
		    parameters,
		    [runtime,
		        groupCount,
		        diagnosticLabel](PassCommandContext& context, TypedPassParameterInstance<typename TShader::Parameters>& passParameters)
		    {
			    const bool valid = passParameters.Sync();
			    assert(valid);
			    const bool dispatched = DispatchComputeShader(
			        context.Resources,
			        context.Commands,
			        runtime->BindingLayout,
			        runtime->Pipeline,
			        passParameters,
			        groupCount,
			        nullptr,
			        0,
			        nullptr,
			        diagnosticLabel.c_str());
			    assert(dispatched);
		    });
	}

	template <typename TShader>
	void DispatchAsync(TypedPassParameterInstance<typename TShader::Parameters>& parameters, const ComputeDispatchDesc& groupCount)
	{
		const ShaderRegistrationDesc& shader = GlobalShader<TShader>::GetRegistration();
		DispatchAsync<TShader>(shader.ShaderName, parameters, groupCount);
	}

	template <typename TShader> void DispatchAsync(
	    std::string_view label,
	    TypedPassParameterInstance<typename TShader::Parameters>& parameters,
	    const ComputeDispatchDesc& groupCount)
	{
		m_renderPassRuntimeCache.MaterializeComputeShaderRuntime<TShader>();
		const ComputePassPipelineRuntime* const runtime = &m_renderPassRuntimeCache.GetComputeShaderRuntime<TShader>();
		const std::string diagnosticLabel(label);
		m_frameGraph.AddAsyncComputePass(
		    diagnosticLabel,
		    parameters,
		    [runtime,
		        groupCount,
		        diagnosticLabel](PassCommandContext& context, TypedPassParameterInstance<typename TShader::Parameters>& passParameters)
		    {
			    const bool valid = passParameters.Sync();
			    assert(valid);
			    const bool dispatched = DispatchComputeShader(
			        context.Resources,
			        context.Commands,
			        runtime->BindingLayout,
			        runtime->Pipeline,
			        passParameters,
			        groupCount,
			        nullptr,
			        0,
			        nullptr,
			        diagnosticLabel.c_str());
			    assert(dispatched);
		    });
	}

	template <typename TRayGenerationShader> void TraceRays(
	    std::string_view label,
	    const RayTracingPipelineComposition& composition,
	    TypedPassParameterInstance<typename TRayGenerationShader::Parameters>& parameters,
	    const RayTracingDispatchDimensions& dimensions)
	{
		const ShaderRegistrationDesc& shader = GlobalShader<TRayGenerationShader>::GetRegistration();
		m_renderPassRuntimeCache.MaterializeRayTracingRuntime<TRayGenerationShader>(composition);
		const RayTracingPassPipelineRuntime runtime = m_renderPassRuntimeCache.GetRayTracingRuntime<TRayGenerationShader>(composition);
		const auto regionEnd = [](const RhiRayTracingShaderTableRegion& region) noexcept
		{
			return region.OffsetInBytes + region.SizeInBytes;
		};
		const std::uint64_t tableSize = std::max(
		    {regionEnd(runtime.ShaderTable.GetRayGenerationRegion()),
		        regionEnd(runtime.ShaderTable.GetMissRegion()),
		        regionEnd(runtime.ShaderTable.GetHitGroupRegion()),
		        regionEnd(runtime.ShaderTable.GetCallableRegion())});
		FrameGraphBufferHandle shaderTable = m_frameGraph.ReservePersistentBuffer(
		    FrameGraphBufferDesc::Create(std::string(shader.ShaderName) + "ShaderTable", tableSize),
		    ResourceState::RayTracingShaderTable);
		m_frameGraph.BindPersistentBuffer(shaderTable, runtime.ShaderTable.GetResource(), ResourceState::RayTracingShaderTable);
		const std::string diagnosticLabel(label);
		m_frameGraph.AddRayTracingPass(
		    diagnosticLabel,
		    parameters,
		    shaderTable,
		    [runtime, dimensions, diagnosticLabel](
		        PassCommandContext& context,
		        TypedPassParameterInstance<typename TRayGenerationShader::Parameters>& passParameters)
		    {
			    const bool valid =
			        passParameters.Sync() && ValidateShaderParameters(passParameters.GetPassParameterSet(), diagnosticLabel.c_str());
			    assert(valid);
			    BindRayTracingShaderPass(
			        context.Commands,
			        context.Resources,
			        runtime.BindingLayout,
			        runtime.Pipeline,
			        passParameters.GetPassParameterSet());
			    context.Commands.TraceRays(
			        TraceRaysDesc{
			            .Pipeline = &runtime.Pipeline,
			            .ShaderTable = &runtime.ShaderTable,
			            .RayGeneration = runtime.ShaderTable.GetRayGenerationRegion(),
			            .Miss = runtime.ShaderTable.GetMissRegion(),
			            .HitGroup = runtime.ShaderTable.GetHitGroupRegion(),
			            .Callable = runtime.ShaderTable.GetCallableRegion(),
			            .Width = dimensions.Width,
			            .Height = dimensions.Height,
			            .Depth = dimensions.Depth});
		    });
	}

	template <typename TShader> TypedPassParameterInstance<typename TShader::Parameters>& AllocParameters()
	{
		const ShaderRegistrationDesc& shader = GlobalShader<TShader>::GetRegistration();
		const ShaderStageVisibility visibility = ShaderParameterLayoutBuilder::GetDefaultVisibility(shader.Stage);
		assert(visibility != ShaderStageVisibility::None && "Graph parameters require a concrete shader stage.");
		return m_frameGraph.AllocParameters<typename TShader::Parameters>(shader.ShaderName.data(), visibility);
	}

	template <typename TParameters> TypedPassParameterInstance<TParameters>& AllocGraphParameters(const char* label)
	{
		return m_frameGraph.AllocParameters<TParameters>(label);
	}

	template <typename TParameters, typename TCallback>
	void AddPassParameterSetup(TypedPassParameterInstance<TParameters>& parameters, TCallback&& callback)
	{
		auto* parameterInstance = &parameters;
		m_frameGraph.m_passParameterSetups.emplace_back(
		    [parameterInstance, setup = std::forward<TCallback>(callback)]() mutable { setup(parameterInstance->GetFields()); });
	}

	template <typename TValue, typename TCallback> void AddParameterSetup(TCallback&& callback)
	{
		m_frameGraph.AddParameterSetup<TValue>(std::forward<TCallback>(callback));
	}

	template <typename TValue, typename TParameters, typename TCallback>
	void AddParameterSetup(TypedPassParameterInstance<TParameters>& parameters, TCallback&& callback)
	{
		auto* parameterInstance = &parameters;
		m_frameGraph.AddParameterSetup<TValue>([parameterInstance, setup = std::forward<TCallback>(callback)](const TValue& value) mutable
		    { setup(parameterInstance->GetFields(), value); });
	}

	template <typename TParameters, typename TCallback> void AddResourceProductionSetup(
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

	template <typename TValue = void> ShaderTexture2D<TValue> CreateSRV(FrameGraphTextureHandle handle) const noexcept
	{
		return m_frameGraph.CreateSRV<TValue>(handle);
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

	ShaderAccelerationStructure CreateAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) const noexcept;
	ShaderRenderTarget CreateRenderTarget(
	    FrameGraphTextureHandle handle,
	    FrameGraphAttachmentLoadAction load,
	    FrameGraphAttachmentStoreAction store) const noexcept;
	ShaderDepthTarget CreateDepthTarget(
	    FrameGraphTextureHandle handle,
	    FrameGraphAttachmentLoadAction load,
	    FrameGraphAttachmentStoreAction store,
	    FrameGraphDepthStencilAccess access) const noexcept;

private:
	FrameGraph& m_frameGraph;
	const RenderPassRuntimeCache& m_renderPassRuntimeCache;
};
