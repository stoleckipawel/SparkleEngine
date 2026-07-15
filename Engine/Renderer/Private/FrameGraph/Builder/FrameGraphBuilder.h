#pragma once

#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class RenderHardwareInterface;
class Window;

class FrameGraphBuilder final
{
  public:
	explicit FrameGraphBuilder(FrameGraph& frameGraph) noexcept;

	template <typename SetupFn, typename ExecuteFn>
	void Execute(std::string_view name, EFrameGraphPassKind kind, SetupFn&& setupFn, ExecuteFn&& executeFn)
	{
		Execute(
		    name,
		    kind,
		    EFrameGraphQueuePreference::Graphics,
		    std::forward<SetupFn>(setupFn),
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename SetupFn, typename ExecuteFn>
	void Execute(
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
		RegisterRasterPass<TPass>(
		    TPass::PassName,
		    parameters,
		    [](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters);
		    });
	}

	template <typename TPass> void Dispatch(typename TPass::ParameterInstance& parameters)
	{
		RegisterComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    [](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters);
		    });
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
		RegisterAsyncComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    [](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters);
		    });
	}

	template <typename TPass>
	void Dispatch(
	    std::string_view name,
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		RegisterComputePass<TPass>(
		    name,
		    parameters,
		    [outputWidth, outputHeight](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters, outputWidth, outputHeight);
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
		RegisterAsyncComputePass<TPass>(
		    name,
		    parameters,
		    [outputWidth, outputHeight](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters, outputWidth, outputHeight);
		    });
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

	ShaderAccelerationStructure Read(FrameGraphAccelerationStructureHandle handle) const noexcept
	{
		return m_frameGraph.Read(handle);
	}

	ShaderRenderTarget CreateRenderTarget(FrameGraphTextureHandle handle) const noexcept;
	ShaderDepthTarget CreateDepthTarget(FrameGraphTextureHandle handle) const noexcept;

	template <typename TValue> ShaderUniform<TValue> Uniform(const TValue& value) const noexcept
	{
		return m_frameGraph.Uniform(value);
	}

	FrameGraph& GetGraph() noexcept { return m_frameGraph; }
	const FrameGraph& GetGraph() const noexcept { return m_frameGraph; }

  private:
	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void RegisterRasterPass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddRasterPass<TPass>(name, parameters, std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void RegisterComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddComputePass<TPass>(name, parameters, std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void RegisterAsyncComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddAsyncComputePass<TPass>(name, parameters, std::forward<ExecuteFn>(executeFn));
	}

	FrameGraph& m_frameGraph;
};

struct FrameGraphDependencies
{
	RenderHardwareInterface& renderHardwareInterface;
	Window& window;
	RenderViewportExtent renderExtent;
	RenderViewportExtent outputExtent;
	bool presentSceneToBackBuffer = true;
};

struct FrameGraphBuildResult
{
	std::unique_ptr<FrameGraph> Graph;
	FrameAssemblyResourceLayout Resources = {};
};

class FrameGraphFactory final
{
  public:
	explicit FrameGraphFactory(const FrameGraphDependencies& dependencies) noexcept;

	FrameGraphBuildResult Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};
