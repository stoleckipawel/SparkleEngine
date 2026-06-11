#pragma once

#include "FrameGraph/FrameGraph.h"

#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

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

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		m_frameGraph.AddComputePass<TPass>(name, parameters, std::forward<ExecuteFn>(executeFn));
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
	FrameGraphTextureHandle CreateTexture(const FrameGraphTextureDesc& desc) noexcept;
	FrameGraphBufferHandle ImportBuffer(const FrameGraphBufferDesc& desc, NativeResourceHandle resource, ResourceState initialState) noexcept;
	FrameGraphBufferHandle ImportPersistentBuffer(
	    const FrameGraphBufferDesc& desc,
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
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
	void ClearPersistentAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) noexcept;

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
	FrameGraphTextureHandle SceneColor;
	FrameGraphTextureHandle FinalSceneColor;
	FrameGraphTextureHandle SceneDepth;
	FrameGraphTextureHandle MotionVectors;
	FrameGraphAccelerationStructureHandle SceneTlas;
};

class FrameGraphFactory final
{
  public:
	explicit FrameGraphFactory(const FrameGraphDependencies& dependencies) noexcept;

	FrameGraphBuildResult Build() const;

  private:
	FrameGraphDependencies m_dependencies;
};
