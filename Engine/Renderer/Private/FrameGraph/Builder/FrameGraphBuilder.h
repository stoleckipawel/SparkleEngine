#pragma once

#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Frame/RayTracing/RayTracingSceneFrameGraphResources.h"
#include "Frame/Core/FrameAssembly.h"
#include "Frame/Core/FrameRenderPath.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class RenderHardwareInterface;
class Window;

class FrameGraphBuilder final
{
  public:
	explicit FrameGraphBuilder(FrameGraph& frameGraph) noexcept;

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(std::string_view name, EFrameGraphPassFlags flags, SetupFn&& setupFn, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddPass(name, flags, std::forward<SetupFn>(setupFn), std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddRasterPass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddRasterPass<TPass>(name, parameters, std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass> void AddRasterShaderPass(typename TPass::ParameterInstance& parameters)
	{
		AddRasterPass<TPass>(
		    TPass::PassName,
		    parameters,
		    [](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters);
		    });
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddComputePass<TPass>(name, parameters, std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass> void AddComputeShaderPass(typename TPass::ParameterInstance& parameters)
	{
		AddComputePass<TPass>(
		    TPass::PassName,
		    parameters,
		    [](PassExecutionContext& context, typename TPass::ParameterInstance& passParameters)
		    {
			    const TPass pass(context.RuntimeServices.GetPassRuntime<TPass>());
			    pass.Execute(context, passParameters);
		    });
	}

	template <typename TPass>
	void AddSizedComputeShaderPass(
	    typename TPass::ParameterInstance& parameters,
	    std::uint32_t outputWidth,
	    std::uint32_t outputHeight)
	{
		AddComputePass<TPass>(
		    TPass::PassName,
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

	template <typename TPass> typename TPass::ParameterInstance& AllocPassParameters()
	{
		return m_frameGraph.AllocPassParameters<TPass>();
	}

	FrameGraphTextureHandle ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	FrameGraphTextureHandle ImportTexture(const FrameGraphTextureDesc& desc, NativeResourceHandle resource, ResourceState initialState) noexcept;
	FrameGraphTextureHandle ImportPersistentTexture(
	    const FrameGraphTextureDesc& desc,
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
	FrameGraphTextureHandle ReservePersistentTexture(
	    const FrameGraphTextureDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphTextureHandle CreateTexture(const FrameGraphTextureDesc& desc) noexcept;
	FrameGraphBufferHandle ImportBuffer(const FrameGraphBufferDesc& desc, NativeResourceHandle resource, ResourceState initialState) noexcept;
	FrameGraphBufferHandle ImportPersistentBuffer(
	    const FrameGraphBufferDesc& desc,
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
	FrameGraphBufferHandle ReservePersistentBuffer(
	    const FrameGraphBufferDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphBufferHandle CreateBuffer(const FrameGraphBufferDesc& desc) noexcept;
	FrameGraphAccelerationStructureHandle ImportAccelerationStructure(
	    const FrameGraphAccelerationStructureDesc& desc,
	    NativeResourceHandle resource,
	    RhiGpuVirtualAddress gpuAddress,
	    ResourceState initialState = ResourceState::RayTracingAccelerationStructure) noexcept;
	FrameGraphAccelerationStructureHandle ImportPersistentAccelerationStructure(
	    const FrameGraphAccelerationStructureDesc& desc,
	    NativeResourceHandle resource,
	    RhiGpuVirtualAddress gpuAddress,
	    ResourceState initialState = ResourceState::RayTracingAccelerationStructure) noexcept;
	FrameGraphAccelerationStructureHandle ReservePersistentAccelerationStructure(
	    const FrameGraphAccelerationStructureDesc& desc,
	    ResourceState initialState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void BindPersistentAccelerationStructure(
	    FrameGraphAccelerationStructureHandle handle,
	    NativeResourceHandle resource,
	    RhiGpuVirtualAddress gpuAddress,
	    ResourceState currentState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void BindPersistentAccelerationStructure(
	    FrameGraphAccelerationStructureHandle handle,
	    RhiOwnedResourceHandle resource,
	    RhiGpuVirtualAddress gpuAddress,
	    ResourceState currentState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void ClearPersistentAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) noexcept;
	void BindPersistentTexture(
	    FrameGraphTextureHandle handle,
	    NativeResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void BindPersistentTexture(
	    FrameGraphTextureHandle handle,
	    RhiOwnedResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void ClearPersistentTextureBinding(FrameGraphTextureHandle handle) noexcept;
	void BindPersistentBuffer(
	    FrameGraphBufferHandle handle,
	    NativeResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void BindPersistentBuffer(
	    FrameGraphBufferHandle handle,
	    RhiOwnedResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void ClearPersistentBufferBinding(FrameGraphBufferHandle handle) noexcept;

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
	FrameGraph& m_frameGraph;
};

struct FrameGraphDependencies
{
	RenderHardwareInterface& renderHardwareInterface;
	Window& window;
	RenderViewportExtent sceneExtent;
	bool presentSceneToBackBuffer = true;
};

struct FrameGraphBuildResult
{
	std::unique_ptr<FrameGraph> Graph;
	FrameAssemblyResourceLayout Resources = {};
	FrameRenderPath RenderPath = FrameRenderPath::RealtimeDeferred;
};

class FrameGraphFactory final
{
  public:
	explicit FrameGraphFactory(const FrameGraphDependencies& dependencies) noexcept;

	FrameGraphBuildResult Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};
