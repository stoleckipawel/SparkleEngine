#pragma once

#include "FrameGraph/FrameGraphPassKind.h"
#include "FrameGraph/FrameGraphQueuePreference.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureDesc.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"
#include "FrameGraph/FrameGraphResourceResolver.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHistory.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Passes/Core/ShaderPass.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Frame/RhiFrameConstants.h"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

class RenderCommandContext;
class RhiCommandSubmissionService;
class FrameExecutionDiagnostics;
class FrameGraphTransientAllocator;
class FrameGraphSubmissionExecutor;
struct PassRuntimeServices;
class Window;
class RenderHardwareInterface;
struct NativeTextureViewInfo;
struct RhiNativeInteropRequest;
struct FrameContext;

class FrameGraph
{
	friend class FrameGraphBatchRecorder;

  private:
	struct AllocatedParameterInstanceBase
	{
		virtual ~AllocatedParameterInstanceBase() noexcept;
	};

	template <typename TParameters>
	struct AllocatedParameterInstance final : AllocatedParameterInstanceBase
	{
		explicit AllocatedParameterInstance(const ShaderParameterStructMetadata<TParameters>& metadata)
		    : Instance(metadata)
		{
		}

		TypedPassParameterInstance<TParameters> Instance;
	};

  public:
	FrameGraph(RenderHardwareInterface* renderHardwareInterface, Window* window);
	~FrameGraph();

	FrameGraph(const FrameGraph&) = delete;
	FrameGraph& operator=(const FrameGraph&) = delete;
	FrameGraph(FrameGraph&&) = delete;
	FrameGraph& operator=(FrameGraph&&) = delete;

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(
	    std::string_view name,
	    EFrameGraphPassKind kind,
	    EFrameGraphQueuePreference queuePreference,
	    SetupFn&& setupFn,
	    ExecuteFn&& executeFn)
	{
		using SetupFnType = std::decay_t<SetupFn>;
		using ExecuteFnType = std::decay_t<ExecuteFn>;

		static_assert(
		    std::is_invocable_v<SetupFnType&, PassResourceBuilder&, const FrameContext&> ||
		        std::is_invocable_v<SetupFnType&, PassResourceBuilder&>,
		    "FrameGraph setup lambda must accept (PassResourceBuilder&, const FrameContext&) or (PassResourceBuilder&).\n");
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, PassExecutionContext&>,
		    "FrameGraph execute lambda must accept (PassExecutionContext&). ");

		SetupFnType normalizedSetup(std::forward<SetupFn>(setupFn));
		ExecuteFnType normalizedExecute(std::forward<ExecuteFn>(executeFn));

		m_passes.push_back(
		    RegisteredPass{
		        .name = std::string(name),
		        .kind = kind,
		        .queuePreference = queuePreference,
		        .setupCallback =
		        [setup = std::move(normalizedSetup)](PassResourceBuilder& builder, const FrameContext& frame) mutable
		        {
			        if constexpr (std::is_invocable_v<SetupFnType&, PassResourceBuilder&, const FrameContext&>)
			        {
				        setup(builder, frame);
			        }
			        else
			        {
				        setup(builder);
			        }

			        return true;
		        },
		        .executeCallback =
		        [execute = std::move(normalizedExecute)](PassExecutionContext& context) mutable
		        {
			        execute(context);
		        }});
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddRasterPass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Raster,
		    EFrameGraphQueuePreference::Graphics,
		    parameters,
		    [](PassResourceBuilder& builder,
		       const TParameterBindings& typedParameters,
		       const FrameContext&,
		       const char* passName)
		    {
			    return RasterShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Compute,
		    EFrameGraphQueuePreference::Graphics,
		    parameters,
		    [](PassResourceBuilder& builder,
		       const TParameterBindings& typedParameters,
		       const FrameContext&,
		       const char* passName)
		    {
			    return ComputeShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddAsyncComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Compute,
		    EFrameGraphQueuePreference::AsyncCompute,
		    parameters,
		    [](PassResourceBuilder& builder,
		       const TParameterBindings& typedParameters,
		       const FrameContext&,
		       const char* passName)
		    {
			    return ComputeShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename ConditionFn, typename ExecuteFn>
	    requires std::is_invocable_r_v<bool, std::decay_t<ConditionFn>&, const FrameContext&> &&
	             std::is_invocable_v<std::decay_t<ExecuteFn>&, PassExecutionContext&, TParameterBindings&>
	void AddConditionalComputePass(
	    std::string_view name,
	    TParameterBindings& parameters,
	    ConditionFn&& condition,
	    ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Compute,
		    EFrameGraphQueuePreference::Graphics,
		    parameters,
		    [condition = std::forward<ConditionFn>(condition)](
		        PassResourceBuilder& builder,
		        const TParameterBindings& typedParameters,
		        const FrameContext& frame,
		        const char* passName) mutable
		    {
			    return condition(frame) &&
			           ComputeShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	void Setup(const FrameContext& frame);

	const FrameGraphPlan& Compile();

	void Execute(
	    const FrameGraphPlan& plan,
	    RhiCommandSubmissionService& submissionService,
	    const FrameContext& frame,
	    const PassRuntimeServices& passRuntimeServices,
	    FrameExecutionDiagnostics& frameDiagnostics) const;
	template <typename TParameters> TypedPassParameterInstance<TParameters>& AllocParameters()
	{
		static const ShaderParameterStructMetadata<TParameters> metadata =
		    ShaderParameterStructBuilder<TParameters>::BuildMetadata("FrameGraphParameters");

		auto allocation = std::make_unique<AllocatedParameterInstance<TParameters>>(metadata);
		TypedPassParameterInstance<TParameters>& instance = allocation->Instance;
		m_allocatedParameterInstances.push_back(std::unique_ptr<AllocatedParameterInstanceBase>(std::move(allocation)));
		return instance;
	}

	FrameGraphTextureHandle ImportBackBuffer(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	FrameGraphTextureHandle ReservePersistentTexture(
	    const FrameGraphTextureDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphTextureHandle CreateTexture(const FrameGraphTextureDesc& desc) noexcept;
	FrameGraphTextureHistory CreateTextureHistory(const FrameGraphTextureDesc& desc) noexcept;
	void InvalidateTextureHistory(FrameGraphTextureHistory history) noexcept;
	bool IsTextureHistoryValid(FrameGraphTextureHistory history) const noexcept;
	FrameGraphBufferHandle ReservePersistentBuffer(
	    const FrameGraphBufferDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphBufferHandle CreateBuffer(const FrameGraphBufferDesc& desc) noexcept;
	FrameGraphAccelerationStructureHandle ReservePersistentAccelerationStructure(
	    const FrameGraphAccelerationStructureDesc& desc,
	    ResourceState initialState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void BindPersistentAccelerationStructure(
	    FrameGraphAccelerationStructureHandle handle,
	    RhiResourceHandle resource,
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
	    RhiResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void BindPersistentTexture(
	    FrameGraphTextureHandle handle,
	    RhiOwnedResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void BindPersistentTexture(
	    FrameGraphTextureHandle handle,
	    RhiOwnedResourceHandle resource,
	    RhiResourceViewHandle shaderResourceView,
	    const FrameGraphTextureDesc& desc,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void ClearPersistentTextureBinding(FrameGraphTextureHandle handle) noexcept;
	void BindPersistentBuffer(
	    FrameGraphBufferHandle handle,
	    RhiResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void BindPersistentBuffer(
	    FrameGraphBufferHandle handle,
	    RhiOwnedResourceHandle resource,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void BindPersistentBuffer(
	    FrameGraphBufferHandle handle,
	    RhiOwnedResourceHandle resource,
	    const FrameGraphBufferDesc& desc,
	    ResourceState currentState = ResourceState::Common) noexcept;
	void ClearPersistentBufferBinding(FrameGraphBufferHandle handle) noexcept;
	void ExportTexture(FrameGraphTextureHandle handle, std::string_view name) noexcept;
	PixelFormat GetTextureFormat(FrameGraphTextureHandle handle) const noexcept;
	ResourceState GetTrackedResourceState(FrameGraphResourceHandle handle) const noexcept;
	void UpdateTrackedResourceState(FrameGraphResourceHandle handle, ResourceState currentState) const noexcept;
	void BindRenderTarget(
	    RenderCommandContext& cmd,
	    FrameGraphTextureHandle renderTargetHandle,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void BindRenderTargets(
	    RenderCommandContext& cmd,
	    std::span<const FrameGraphTextureHandle> renderTargetHandles,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void CopyTexture(RenderCommandContext& cmd, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle)
	    const noexcept;
	void CopyBuffer(RenderCommandContext& cmd, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle)
	    const noexcept;
	void ClearRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept;
	void ClearDepthStencil(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept;
	RhiResourceHandle ResolveResource(FrameGraphTextureHandle handle) const noexcept;
	NativeTextureViewInfo ResolveNativeTextureView(
	    FrameGraphTextureHandle handle,
	    ResourceState state,
	    const RhiNativeInteropRequest& request) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuVirtualAddress ResolveAccelerationStructureGpuAddress(FrameGraphAccelerationStructureHandle handle) const noexcept;

	template <typename TValue = void> ShaderTexture2D<TValue> Read(FrameGraphTextureHandle handle) const noexcept
	{
		ShaderTexture2D<TValue> field;
		field = handle;
		return field;
	}

	template <typename TValue = void> ShaderTexture2D<TValue> CreateSRV(FrameGraphTextureHandle handle) const noexcept
	{
		return Read<TValue>(handle);
	}

	template <typename TValue = void> ShaderBuffer<TValue> Read(FrameGraphBufferHandle handle) const noexcept
	{
		ShaderBuffer<TValue> field;
		field = handle;
		return field;
	}

	template <typename TValue = void> ShaderBuffer<TValue> CreateSRV(FrameGraphBufferHandle handle) const noexcept
	{
		return Read<TValue>(handle);
	}

	template <typename TValue = void> ShaderRWTexture2D<TValue> CreateUAV(FrameGraphTextureHandle handle) const noexcept
	{
		ShaderRWTexture2D<TValue> field;
		field = handle;
		return field;
	}

	template <typename TValue = void> ShaderRWBuffer<TValue> CreateUAV(FrameGraphBufferHandle handle) const noexcept
	{
		ShaderRWBuffer<TValue> field;
		field = handle;
		return field;
	}

	ShaderAccelerationStructure Read(FrameGraphAccelerationStructureHandle handle) const noexcept;
	ShaderRenderTarget CreateRenderTarget(FrameGraphTextureHandle handle) const noexcept;
	ShaderDepthTarget CreateDepthTarget(FrameGraphTextureHandle handle) const noexcept;

	template <typename TValue> ShaderUniform<TValue> Uniform(const TValue& value) const noexcept
	{
		ShaderUniform<TValue> field;
		field = value;
		return field;
	}

  private:
	using SetupCallback = std::function<bool(PassResourceBuilder&, const FrameContext&)>;
	using ExecuteCallback = std::function<void(PassExecutionContext&)>;

	template <typename TParameterBindings, typename ExecuteFn>
	static ExecuteCallback MakeParameterizedExecuteCallback(TParameterBindings* parameters, ExecuteFn&& executeFn)
	{
		using ExecuteFnType = std::decay_t<ExecuteFn>;
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, PassExecutionContext&, TParameterBindings&>,
		    "Typed pass execute lambda must accept (PassExecutionContext&, Parameters&). ");

		ExecuteFnType callback(std::forward<ExecuteFn>(executeFn));
		return [parameters, callback = std::move(callback)](PassExecutionContext& context) mutable
		{
			callback(context, *parameters);
		};
	}

	template <typename TParameterBindings, typename SetupFn, typename ExecuteFn>
	void AddTypedShaderPass(
	    std::string_view name,
	    EFrameGraphPassKind kind,
	    EFrameGraphQueuePreference queuePreference,
	    TParameterBindings& parameters,
	    SetupFn&& setupFn,
	    ExecuteFn&& executeFn)
	{
		auto* parameterBindings = &parameters;
		std::string passName(name);

		m_passes.push_back(
		    RegisteredPass{
		        .name = std::string(name),
		        .kind = kind,
		        .queuePreference = queuePreference,
		        .setupCallback =
		        [parameterBindings, passName, setupFn = std::forward<SetupFn>(setupFn)](
		            PassResourceBuilder& builder,
		            const FrameContext& frame) mutable
		        {
			        return setupFn(builder, *parameterBindings, frame, passName.c_str());
		        },
		        .executeCallback =
		        MakeParameterizedExecuteCallback(
		            parameterBindings,
		            [executeFn = std::forward<ExecuteFn>(executeFn)](
		                PassExecutionContext& context,
		                TParameterBindings& typedParameters) mutable
		            {
			            executeFn(context, typedParameters);
		            })});
	}

	RhiCpuDescriptorHandle ResolveRenderTargetView(FrameGraphResourceHandle handle) const noexcept;
	RhiCpuDescriptorHandle ResolveDepthStencilView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuVirtualAddress ResolveAccelerationStructureGpuAddress(FrameGraphResourceHandle handle) const noexcept;
	std::array<float, 4> GetClearColor(FrameGraphResourceHandle handle) const noexcept;
	float GetClearDepth(FrameGraphResourceHandle handle) const noexcept;
	RhiResourceHandle ResolveResource(FrameGraphResourceHandle handle) const noexcept;
	NativeTextureViewInfo ResolveNativeTextureView(
	    FrameGraphResourceHandle handle,
	    ResourceState state,
	    const RhiNativeInteropRequest& request) const noexcept;
	void CopyResource(RenderCommandContext& cmd, FrameGraphResourceHandle destinationHandle, FrameGraphResourceHandle sourceHandle)
	    const noexcept;
	void SyncImportedResourceAccesses() const noexcept;
	void BuildTransientMaterializationPlan(FrameGraphPlan& plan) const noexcept;
	void EnsureTransientResourcesMaterialized(const FrameGraphPlan& plan) const noexcept;
	void ReleaseExternalResourceViews() noexcept;
	void ReleaseExternalResourceViews(FrameGraphResourceHandle handle) noexcept;
	void EmitTransientAliasingBarriers(RenderCommandContext& cmd, const std::vector<FrameGraphAliasingBarrier>& barriers) const noexcept;
	void EmitTransientAliasingBarriers(
	    RenderCommandContext& cmd,
	    std::string_view passName,
	    const std::vector<FrameGraphAliasingBarrier>& barriers) const noexcept;
	void EmitCompiledBarriers(RenderCommandContext& cmd, const std::vector<FrameGraphBarrier>& barriers) const noexcept;
	void EmitCompiledBarriers(RenderCommandContext& cmd, std::string_view passName, const std::vector<FrameGraphBarrier>& barriers)
	    const noexcept;
	void RecordFrameBeginBarriers(
	    const FrameGraphPlan& plan,
	    RenderCommandList& commandList,
	    FrameExecutionDiagnostics& frameDiagnostics) const;
	void RecordFrameEndBarriers(
	    const FrameGraphPlan& plan,
	    RenderCommandList& commandList,
	    FrameExecutionDiagnostics& frameDiagnostics) const;
	void PrepareTextureHistories(const FrameGraphPlan& plan);
	void CommitTextureHistories() const noexcept;
	void ReleaseTextureHistories() noexcept;
	FrameGraphResourceHandle AllocateDynamicResourceHandle() noexcept;

	struct TextureHistoryRecord
	{
		FrameGraphTextureHistory handles = {};
		FrameGraphTextureDesc desc = {};
		std::array<RhiOwnedResourceHandle, RhiFrameConstants::FramesInFlight> resources = {};
		std::array<ResourceState, RhiFrameConstants::FramesInFlight> states = {};
		std::array<std::uint64_t, RhiFrameConstants::FramesInFlight> generations = {};
		std::uint64_t generation = 1u;
		std::uint32_t previousIndex = 0u;
		std::uint32_t currentIndex = 0u;
		bool usedThisFrame = false;
		bool writtenThisFrame = false;
		bool allowRenderTarget = false;
		bool allowDepthStencil = false;
		bool allowUnorderedAccess = false;
	};

	struct VirtualTransientResource
	{
		FrameGraphResourceHandle handle;
		FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
		FrameGraphTextureDesc textureDesc{};
		FrameGraphBufferDesc bufferDesc{};
	};

	struct RegisteredPass
	{
		std::string name;
		EFrameGraphPassKind kind = EFrameGraphPassKind::None;
		EFrameGraphQueuePreference queuePreference = EFrameGraphQueuePreference::Graphics;
		SetupCallback setupCallback;
		ExecuteCallback executeCallback;
		bool active = true;
	};

	std::vector<RegisteredPass> m_passes;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	Window* m_window = nullptr;
	FrameGraphResourceRegistry m_resourceRegistry;
	mutable FrameGraphResourceStateTracker m_resourceStateTracker;
	mutable FrameGraphResourceResolver m_resourceResolver;
	FrameGraphPlan m_compiledPlan;
	mutable std::vector<RhiSubmissionToken> m_submissionBatchTokens;
	std::vector<FrameGraphProductRoot> m_productRoots;
	std::uint32_t m_nextDynamicResourceIndex = 0;
	std::vector<VirtualTransientResource> m_virtualTransientResources;
	mutable std::vector<TextureHistoryRecord> m_textureHistories;
	mutable std::unique_ptr<FrameGraphTransientAllocator> m_transientAllocator;
	std::vector<std::unique_ptr<AllocatedParameterInstanceBase>> m_allocatedParameterInstances;
};
