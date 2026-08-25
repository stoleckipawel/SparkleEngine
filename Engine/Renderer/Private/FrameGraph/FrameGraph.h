#pragma once

#include "FrameGraph/FrameGraphPassKind.h"
#include "FrameGraph/FrameGraphQueuePreference.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"
#include "FrameGraph/FrameGraphResourceResolver.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "FrameGraph/FrameGraphRasterPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "FrameGraph/Execution/PassCommandContext.h"
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
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class RenderCommandContext;
class RenderCommandList;
class RhiCommandSubmissionService;
class FrameExecutionDiagnostics;
class FrameGraphTransientAllocator;
class FrameGraphSubmissionExecutor;
class FrameGraphRecordingChunkRecorder;
class FrameGraphBuilder;
class TaskExecutor;
class Window;
class RenderHardwareInterface;
class RasterPassRenderState;
struct NativeTextureViewInfo;
struct RhiNativeInteropRequest;
class FrameGraph
{
	friend class FrameGraphRecordingChunkRecorder;
	friend class FrameGraphResourceCommands;
	friend class FrameGraphBuilder;

private:
	struct VirtualTransientResource;

	struct AllocatedParameterInstanceBase
	{
		virtual ~AllocatedParameterInstanceBase() noexcept;
	};

	template <typename TParameters> struct AllocatedParameterInstance final : AllocatedParameterInstanceBase
	{
		explicit AllocatedParameterInstance(const ShaderParameterStructMetadata<TParameters>& metadata) :
		    Instance(metadata)
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

	template <typename SetupFn, typename ExecuteFn> void AddPass(
	    std::string_view name,
	    EFrameGraphPassKind kind,
	    EFrameGraphQueuePreference queuePreference,
	    SetupFn&& setupFn,
	    ExecuteFn&& executeFn)
	{
		using SetupFnType = std::decay_t<SetupFn>;
		using ExecuteFnType = std::decay_t<ExecuteFn>;

		static_assert(
		    std::is_invocable_v<SetupFnType&, PassResourceBuilder&>,
		    "FrameGraph setup lambda must accept (PassResourceBuilder&).\n");
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, PassCommandContext&>,
		    "FrameGraph execute lambda must accept (PassCommandContext&). ");

		SetupFnType normalizedSetup(std::forward<SetupFn>(setupFn));
		ExecuteFnType normalizedExecute(std::forward<ExecuteFn>(executeFn));

		m_passes.push_back(
		    RegisteredPass{
		        .name = std::string(name),
		        .kind = kind,
		        .queuePreference = queuePreference,
		        .setupCallback =
		            [setup = std::move(normalizedSetup)](PassResourceBuilder& builder) mutable
		        {
			        setup(builder);
			        return true;
		        },
		        .executeCallback = [execute = std::move(normalizedExecute)](PassCommandContext& context) mutable { execute(context); }});
	}

	template <typename TParameterBindings, typename ExecuteFn>
	requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassCommandContext&, TParameterBindings&>
	void AddRasterPass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Raster,
		    EFrameGraphQueuePreference::Graphics,
		    parameters,
		    [](PassResourceBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    { return SetupShaderParameters(builder, typedParameters, passName); },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TParameterBindings, typename ExecuteFn>
	requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassCommandContext&, TParameterBindings&>
	void AddComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Compute,
		    EFrameGraphQueuePreference::Graphics,
		    parameters,
		    [](PassResourceBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    { return SetupShaderParameters(builder, typedParameters, passName); },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TParameterBindings, typename ExecuteFn>
	requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassCommandContext&, TParameterBindings&>
	void AddAsyncComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::Compute,
		    EFrameGraphQueuePreference::AsyncCompute,
		    parameters,
		    [](PassResourceBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    { return SetupShaderParameters(builder, typedParameters, passName); },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TParameterBindings, typename ExecuteFn>
	requires std::is_invocable_v<std::decay_t<ExecuteFn>&, PassCommandContext&, TParameterBindings&>
	void AddRayTracingPass(std::string_view name, TParameterBindings& parameters, FrameGraphBufferHandle shaderTable, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassKind::RayTracing,
		    EFrameGraphQueuePreference::Graphics,
		    parameters,
		    [shaderTable](PassResourceBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    {
			    const bool parametersValid = SetupShaderParameters(builder, typedParameters, passName);
			    builder.Read(shaderTable, ResourceUsage::RayTracingShaderTableRead, "ShaderTable");
			    return parametersValid;
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	void Setup();
	void ApplyPassParameterDefaults();
	void PreparePasses();
	void ApplyResourceProductionSetups();

	template <typename TValue, typename TCallback> void AddParameterSetup(TCallback&& callback)
	{
		auto& callbacks = m_parameterSetups[typeid(TValue)];
		callbacks.emplace_back(
		    [setup = std::forward<TCallback>(callback)](const void* value) mutable { setup(*static_cast<const TValue*>(value)); });
	}

	template <typename TValue> void ApplyParameters(const TValue& value)
	{
		const auto callbacks = m_parameterSetups.find(typeid(TValue));
		if (callbacks == m_parameterSetups.end())
		{
			return;
		}
		for (const ParameterSetupCallback& setup : callbacks->second)
		{
			setup(&value);
		}
	}

	const FrameGraphPlan& Compile();

	void Execute(
	    const FrameGraphPlan& plan,
	    RhiCommandSubmissionService& submissionService,
	    FrameExecutionDiagnostics& frameDiagnostics,
	    TaskExecutor& taskExecutor) const;
	template <typename TParameters> TypedPassParameterInstance<TParameters>& AllocParameters(
	    const char* debugName,
	    ShaderStageVisibility visibility = ShaderStageVisibility::None)
	{
		static const ShaderParameterStructMetadata<TParameters> metadata =
		    ShaderParameterStructBuilder<TParameters>::BuildMetadata(debugName, visibility);

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
	bool HasBeenProduced(FrameGraphResourceHandle handle) const noexcept;
	bool HasBeenProduced(FrameGraphTextureHandle handle) const noexcept { return HasBeenProduced(handle.GetResourceHandle()); }
	bool HasBeenProduced(FrameGraphBufferHandle handle) const noexcept { return HasBeenProduced(handle.GetResourceHandle()); }
	bool HasBeenProduced(FrameGraphAccelerationStructureHandle handle) const noexcept
	{
		return HasBeenProduced(handle.GetResourceHandle());
	}
	FrameGraphBufferHandle ReservePersistentBuffer(
	    const FrameGraphBufferDesc& desc,
	    ResourceState initialState = ResourceState::Common) noexcept;
	FrameGraphBufferHandle CreateBuffer(const FrameGraphBufferDesc& desc) noexcept;
	FrameGraphAccelerationStructureHandle ReservePersistentAccelerationStructure(
	    std::string_view name,
	    ResourceState initialState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void BindPersistentAccelerationStructure(
	    FrameGraphAccelerationStructureHandle handle,
	    RhiResourceHandle resource,
	    ResourceState currentState = ResourceState::RayTracingAccelerationStructure) noexcept;
	void BindPersistentAccelerationStructure(
	    FrameGraphAccelerationStructureHandle handle,
	    RhiOwnedResourceHandle resource,
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
	    RenderCommandContext& commandContext,
	    FrameGraphTextureHandle renderTargetHandle,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void BindRenderTargets(
	    RenderCommandContext& commandContext,
	    std::span<const FrameGraphTextureHandle> renderTargetHandles,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void CopyTexture(
	    RenderCommandContext& commandContext,
	    FrameGraphTextureHandle destinationHandle,
	    FrameGraphTextureHandle sourceHandle) const noexcept;
	void CopyBuffer(
	    RenderCommandContext& commandContext,
	    FrameGraphBufferHandle destinationHandle,
	    FrameGraphBufferHandle sourceHandle) const noexcept;
	void ClearRenderTarget(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept;
	void ClearDepthStencil(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept;
	RhiResourceHandle ResolveResource(FrameGraphTextureHandle handle) const noexcept;
	NativeTextureViewInfo ResolveNativeTextureView(
	    FrameGraphTextureHandle handle,
	    ResourceState state,
	    const RhiNativeInteropRequest& request) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept;
	RhiResourceHandle ResolveAccelerationStructure(FrameGraphAccelerationStructureHandle handle) const noexcept;

	template <typename TValue = void> ShaderTexture2D<TValue> CreateSRV(FrameGraphTextureHandle handle) const noexcept
	{
		ShaderTexture2D<TValue> field;
		field = handle;
		return field;
	}

	template <typename TValue = void> ShaderBuffer<TValue> CreateSRV(FrameGraphBufferHandle handle) const noexcept
	{
		ShaderBuffer<TValue> field;
		field = handle;
		return field;
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

	ShaderAccelerationStructure CreateAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) const noexcept;
	FrameGraphRasterPass BuildRasterPass(const PassParameterSet& parameters, const RasterPassRenderState& renderState) const;

	template <typename TValue> ShaderUniform<TValue> Uniform(const TValue& value) const noexcept
	{
		ShaderUniform<TValue> field;
		field = value;
		return field;
	}

private:
	using SetupCallback = std::function<bool(PassResourceBuilder&)>;
	using ExecuteCallback = std::function<void(PassCommandContext&)>;
	using PassParameterSetupCallback = std::function<void()>;
	using PassPreparationCallback = std::function<void()>;
	using PassPreparation = std::pair<FrameGraphPassIndex, PassPreparationCallback>;
	using ResourceProductionSetupCallback = std::function<void()>;
	using ParameterSetupCallback = std::function<void(const void*)>;

	template <typename TParameterBindings, typename ExecuteFn>
	static ExecuteCallback MakeParameterizedExecuteCallback(TParameterBindings* parameters, ExecuteFn&& executeFn)
	{
		using ExecuteFnType = std::decay_t<ExecuteFn>;
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, PassCommandContext&, TParameterBindings&>,
		    "Typed pass execute lambda must accept (PassCommandContext&, Parameters&). ");

		ExecuteFnType callback(std::forward<ExecuteFn>(executeFn));
		return [parameters, callback = std::move(callback)](PassCommandContext& context) mutable
		{
			callback(context, *parameters);
		};
	}

	template <typename TParameterBindings, typename SetupFn, typename ExecuteFn> void AddTypedShaderPass(
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
		        .executionModel = FrameGraphPassExecutionModel::TypedShader,
		        .setupCallback = [parameterBindings, passName, setupFn = std::forward<SetupFn>(setupFn)](
		                             PassResourceBuilder& builder) mutable
		        { return setupFn(builder, *parameterBindings, passName.c_str()); },
		        .executeCallback = MakeParameterizedExecuteCallback(
		            parameterBindings,
		            [executeFn =
		                    std::forward<ExecuteFn>(executeFn)](PassCommandContext& context, TParameterBindings& typedParameters) mutable
		            { executeFn(context, typedParameters); })});
	}

	RhiCpuDescriptorHandle ResolveRenderTargetView(FrameGraphResourceHandle handle) const noexcept;
	RhiCpuDescriptorHandle ResolveDepthStencilView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept;
	std::array<float, 4> GetClearColor(FrameGraphResourceHandle handle) const noexcept;
	float GetClearDepth(FrameGraphResourceHandle handle) const noexcept;
	RhiResourceHandle ResolveResource(FrameGraphResourceHandle handle) const noexcept;
	NativeTextureViewInfo ResolveNativeTextureView(
	    FrameGraphResourceHandle handle,
	    ResourceState state,
	    const RhiNativeInteropRequest& request) const noexcept;
	void CopyResource(
	    RenderCommandContext& commandContext,
	    FrameGraphResourceHandle destinationHandle,
	    FrameGraphResourceHandle sourceHandle) const noexcept;
	void SyncImportedResourceAccesses() const noexcept;
	void BuildTransientMaterializationPlan(FrameGraphPlan& plan) const noexcept;
	FrameGraphTransientResourcePlan BuildTransientResourcePlan(
	    const VirtualTransientResource& transientResource,
	    const FrameGraphResourceMetadata& resourceMetadata,
	    const FrameGraphPlan& plan) const noexcept;
	void EnsureTransientResourcesMaterialized(const FrameGraphPlan& plan) const noexcept;
	void ReleaseExternalResourceViews() noexcept;
	void ReleaseExternalResourceViews(FrameGraphResourceHandle handle) noexcept;
	void EmitTransientAliasingBarriers(
	    RenderCommandContext& commandContext,
	    const std::vector<FrameGraphAliasingBarrier>& barriers) const noexcept;
	void EmitTransientAliasingBarriers(
	    RenderCommandContext& commandContext,
	    std::string_view passName,
	    const std::vector<FrameGraphAliasingBarrier>& barriers) const noexcept;
	void EmitCompiledBarriers(RenderCommandContext& commandContext, const std::vector<FrameGraphBarrier>& barriers) const noexcept;
	void EmitCompiledBarriers(
	    RenderCommandContext& commandContext,
	    std::string_view passName,
	    const std::vector<FrameGraphBarrier>& barriers) const noexcept;
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
		std::array<RhiOwnedResourceHandle, RhiFrameConstants::MaxFrameSlotCount> resources = {};
		std::array<ResourceState, RhiFrameConstants::MaxFrameSlotCount> states = {};
		std::array<std::uint64_t, RhiFrameConstants::MaxFrameSlotCount> generations = {};
		std::uint64_t generation = 1u;
		std::uint32_t previousIndex = 0u;
		std::uint32_t currentIndex = 0u;
		bool usedThisFrame = false;
		bool writtenThisFrame = false;
		bool allowRenderTarget = false;
		bool allowDepthStencil = false;
		bool allowUnorderedAccess = false;
	};
	mutable std::uint64_t m_historyFrameIndex = 0;

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
		FrameGraphPassExecutionModel executionModel = FrameGraphPassExecutionModel::Callback;
		SetupCallback setupCallback;
		ExecuteCallback executeCallback;
		bool active = true;
	};

	std::vector<RegisteredPass> m_passes;
	std::vector<PassParameterSetupCallback> m_passParameterSetups;
	std::vector<PassPreparation> m_passPreparations;
	std::vector<ResourceProductionSetupCallback> m_resourceProductionSetups;
	std::unordered_map<std::type_index, std::vector<ParameterSetupCallback>> m_parameterSetups;
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
