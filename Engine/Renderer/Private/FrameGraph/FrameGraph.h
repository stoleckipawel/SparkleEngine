#pragma once

#include "FrameGraph/FrameGraphPassFlags.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "FrameGraph/Builder/PassBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "FrameGraph/ResourceRegistry.h"
#include "FrameGraph/Execution/RenderGraphPassContext.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Passes/ShaderPass.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

class RenderCommandContext;
class FrameExecutionDiagnostics;
class FrameGraphTransientAllocator;
struct RenderPassContext;
class Window;
struct FrameContext;

class FrameGraph
{
  public:
	struct CompiledPlan;

  private:
	struct AllocatedParameterInstanceBase
	{
		virtual ~AllocatedParameterInstanceBase() noexcept = default;
	};

  public:
	FrameGraph(RenderHardwareInterface* renderHardwareInterface, Window* window, RenderViewportExtent sceneExtent);
	~FrameGraph();

	FrameGraph(const FrameGraph&) = delete;
	FrameGraph& operator=(const FrameGraph&) = delete;
	FrameGraph(FrameGraph&&) = delete;
	FrameGraph& operator=(FrameGraph&&) = delete;

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(std::string_view name, EFrameGraphPassFlags flags, SetupFn&& setupFn, ExecuteFn&& executeFn)
	{
		using SetupFnType = std::decay_t<SetupFn>;
		using ExecuteFnType = std::decay_t<ExecuteFn>;

		static_assert(
		    std::is_invocable_v<SetupFnType&, PassBuilder&, const FrameContext&> || std::is_invocable_v<SetupFnType&, PassBuilder&>,
		    "FrameGraph setup lambda must accept (PassBuilder&, const FrameContext&) or (PassBuilder&).\n");
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, RenderGraphPassContext&>,
		    "FrameGraph execute lambda must accept (RenderGraphPassContext&). ");

		SetupFnType normalizedSetup(std::forward<SetupFn>(setupFn));
		ExecuteFnType normalizedExecute(std::forward<ExecuteFn>(executeFn));

		m_passes.push_back(
		    RegisteredPass{
		        std::string(name),
		        flags,
		        [setup = std::move(normalizedSetup)](PassBuilder& builder, const FrameContext& frame) mutable
		        {
			        if constexpr (std::is_invocable_v<SetupFnType&, PassBuilder&, const FrameContext&>)
			        {
				        setup(builder, frame);
			        }
			        else
			        {
				        setup(builder);
			        }
		        },
		        [execute = std::move(normalizedExecute)](RenderGraphPassContext& context) mutable
		        {
			        execute(context);
		        }});
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, RenderGraphPassContext&, TParameterBindings&>
	void AddRasterPass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassFlags::Raster,
		    parameters,
		    [](PassBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    {
			    return RasterShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename... TExecuteArgs>
	    requires std::is_invocable_v<decltype(&TPass::Execute), RenderGraphPassContext&, TParameterBindings&, TExecuteArgs...>
	void AddRasterPass(std::string_view name, TParameterBindings& parameters, TExecuteArgs&&... executeArgs)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassFlags::Raster,
		    parameters,
		    [](PassBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    {
			    return RasterShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    MakeDirectPassExecuteCallback<TPass, TParameterBindings>(std::forward<TExecuteArgs>(executeArgs)...));
	}

	template <typename TPass, typename TParameterBindings, typename ExecuteFn>
	    requires std::is_invocable_v<std::decay_t<ExecuteFn>&, RenderGraphPassContext&, TParameterBindings&>
	void AddComputePass(std::string_view name, TParameterBindings& parameters, ExecuteFn&& executeFn)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassFlags::Compute,
		    parameters,
		    [](PassBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    {
			    return ComputeShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    std::forward<ExecuteFn>(executeFn));
	}

	template <typename TPass, typename TParameterBindings, typename... TExecuteArgs>
	    requires std::is_invocable_v<decltype(&TPass::Execute), RenderGraphPassContext&, TParameterBindings&, TExecuteArgs...>
	void AddComputePass(std::string_view name, TParameterBindings& parameters, TExecuteArgs&&... executeArgs)
	{
		AddTypedShaderPass(
		    name,
		    EFrameGraphPassFlags::Compute,
		    parameters,
		    [](PassBuilder& builder, const TParameterBindings& typedParameters, const char* passName)
		    {
			    return ComputeShaderPass<typename TPass::Parameters>::Setup(builder, typedParameters, passName);
		    },
		    MakeDirectPassExecuteCallback<TPass, TParameterBindings>(std::forward<TExecuteArgs>(executeArgs)...));
	}

	void Setup(const FrameContext& frame);

	CompiledPlan Compile();

	void Execute(
	    const CompiledPlan& plan,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const RenderPassContext& renderPassContext,
	    FrameExecutionDiagnostics& frameDiagnostics) const;
	template <typename TParameters> TypedPassParameterInstance<TParameters>& AllocParameters()
	{
		struct AllocatedParameterInstance final : AllocatedParameterInstanceBase
		{
			explicit AllocatedParameterInstance(const ShaderParameterStructMetadata<TParameters>& metadata) : Instance(metadata) {}

			TypedPassParameterInstance<TParameters> Instance;
		};

		static const ShaderParameterStructMetadata<TParameters> metadata =
		    ShaderParameterStructBuilder<TParameters>::BuildMetadata("FrameGraphParameters");

		auto allocation = std::make_unique<AllocatedParameterInstance>(metadata);
		TypedPassParameterInstance<TParameters>& instance = allocation->Instance;
		m_allocatedParameterInstances.push_back(std::unique_ptr<AllocatedParameterInstanceBase>(std::move(allocation)));
		return instance;
	}

	template <typename TPass> typename TPass::ParameterInstance& AllocPassParameters()
	{
		using Parameters = typename TPass::Parameters;
		using ParameterInstance = typename TPass::ParameterInstance;

		struct AllocatedPassParameterInstance final : AllocatedParameterInstanceBase
		{
			explicit AllocatedPassParameterInstance(const ShaderParameterStructMetadata<Parameters>& metadata) : Instance(metadata) {}

			ParameterInstance Instance;
		};

		auto allocation = std::make_unique<AllocatedPassParameterInstance>(TPass::GetParameterMetadata());
		ParameterInstance& instance = allocation->Instance;
		m_allocatedParameterInstances.push_back(std::unique_ptr<AllocatedParameterInstanceBase>(std::move(allocation)));
		return instance;
	}

	FrameGraphTextureHandle ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	FrameGraphTextureHandle ImportTexture(const FrameGraphTextureDesc& desc, NativeResourceHandle resource, ResourceState initialState) noexcept;
	FrameGraphTextureHandle CreateTexture(const FrameGraphTextureDesc& desc) noexcept;
	FrameGraphBufferHandle ImportBuffer(const FrameGraphBufferDesc& desc, NativeResourceHandle resource, ResourceState initialState) noexcept;
	FrameGraphBufferHandle CreateBuffer(const FrameGraphBufferDesc& desc) noexcept;
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
	void CopyTexture(RenderCommandContext& cmd, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle) const noexcept;
	void CopyBuffer(RenderCommandContext& cmd, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle) const noexcept;
	void ClearRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept;
	void ClearDepthStencil(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept;
	NativeResourceHandle ResolveResource(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept;

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

	template <typename TValue = void> ShaderBuffer<TValue> CreateSRV(FrameGraphBufferHandle handle) const noexcept { return Read<TValue>(handle); }

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

	ShaderRenderTarget CreateRenderTarget(FrameGraphTextureHandle handle) const noexcept
	{
		ShaderRenderTarget field;
		field = handle;
		return field;
	}

	ShaderDepthTarget CreateDepthTarget(FrameGraphTextureHandle handle) const noexcept
	{
		ShaderDepthTarget field;
		field = handle;
		return field;
	}

	template <typename TValue> ShaderUniform<TValue> Uniform(const TValue& value) const noexcept
	{
		ShaderUniform<TValue> field;
		field = value;
		return field;
	}

	using PassIndex = std::uint32_t;
	using ResourceIndex = std::uint32_t;
	static constexpr PassIndex INVALID_PASS_INDEX = static_cast<PassIndex>(-1);
	static constexpr ResourceIndex INVALID_RESOURCE_INDEX = static_cast<ResourceIndex>(-1);

	struct CompiledBarrier
	{
		enum class Type : std::uint8_t
		{
			Transition,
			UnorderedAccess
		};

		FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
		Type type = Type::Transition;
		ResourceState before = ResourceState::Common;
		ResourceState after = ResourceState::Common;
	};

	struct CompiledAliasingBarrier
	{
		std::uint32_t physicalBlockIndex = INVALID_RESOURCE_INDEX;
		FrameGraphResourceHandle beforeHandle = FrameGraphResourceHandle::Invalid();
		FrameGraphResourceHandle afterHandle = FrameGraphResourceHandle::Invalid();
		PassIndex executeBeforePass = INVALID_PASS_INDEX;
		PassIndex executeAfterPass = INVALID_PASS_INDEX;
	};

	struct ResourceVersion
	{
		FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
		std::uint32_t version = 0;
		PassIndex writerPass = INVALID_PASS_INDEX;
		std::vector<PassIndex> readerPasses;
	};

	struct CompilePassRecord
	{
		PassIndex index = INVALID_PASS_INDEX;
		std::string passName;
		EFrameGraphPassFlags flags = EFrameGraphPassFlags::None;
		EFrameGraphPassFlags passKind = EFrameGraphPassFlags::None;
		std::string diagnosticName;
		std::string displayLabel;
		std::string eventScopeLabel;
		std::vector<PassResourceDeclaration> declarations;
		std::vector<PassIndex> dependsOn;
		std::vector<PassIndex> successors;
		std::uint32_t inDegree = 0;
		bool alive = true;
		std::vector<CompiledAliasingBarrier> compiledAliasingBarriers;
		std::vector<CompiledBarrier> compiledBarriers;
	};

	struct CompileResourceEntry
	{
		ResourceIndex index = INVALID_RESOURCE_INDEX;
		FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
		FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
		FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
		FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
		ResourceState initialState = ResourceState::Common;
		ResourceState finalState = ResourceState::Common;
		ResourceState currentState = ResourceState::Common;
		std::string debugName;
		std::uint32_t currentVersion = 0;
		std::vector<ResourceVersion> versions;
	};

	struct CompiledTransientResourcePlan
	{
		enum class AllocationPool : std::uint8_t
		{
			Color,
			Depth,
			Buffer
		};

		struct PhysicalAllocationPlan
		{
			std::uint32_t allocationIndex = INVALID_RESOURCE_INDEX;
			std::uint32_t physicalBlockIndex = INVALID_RESOURCE_INDEX;
			AllocationPool pool = AllocationPool::Color;
			std::uint64_t sizeInBytes = 0;
			std::uint64_t alignment = 0;
			std::uint64_t heapOffset = 0;
			RhiTextureResourceDesc textureResourceDesc{};
			RhiBufferResourceDesc bufferResourceDesc{};
			RhiOptimizedClearValue optimizedClearValue{};
			bool hasOptimizedClearValue = false;
			ResourceState initialState = ResourceState::Common;
		};

		FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
		FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
		FrameGraphTextureDesc textureDesc{};
		FrameGraphBufferDesc bufferDesc{};
		FrameGraphResourceKind kind = FrameGraphResourceKind::ColorRenderTarget;
		PhysicalAllocationPlan physicalAllocation{};
		PassIndex firstUserPass = INVALID_PASS_INDEX;
		PassIndex lastUserPass = INVALID_PASS_INDEX;
		PassIndex firstExecutionIndex = INVALID_PASS_INDEX;
		PassIndex lastExecutionIndex = INVALID_PASS_INDEX;
		bool readUsed = false;
		bool writeUsed = false;
		std::vector<ResourceState> requiredStates;
	};

	struct CompiledPhysicalBlockPlan
	{
		std::uint32_t physicalBlockIndex = INVALID_RESOURCE_INDEX;
		CompiledTransientResourcePlan::AllocationPool pool = CompiledTransientResourcePlan::AllocationPool::Color;
		std::uint64_t sizeInBytes = 0;
		std::uint64_t alignment = 0;
		std::uint64_t heapOffset = 0;
		RhiTextureResourceDesc textureResourceDesc{};
		RhiBufferResourceDesc bufferResourceDesc{};
		RhiOptimizedClearValue optimizedClearValue{};
		bool hasOptimizedClearValue = false;
		PassIndex firstExecutionIndex = INVALID_PASS_INDEX;
		PassIndex lastExecutionIndex = INVALID_PASS_INDEX;
		std::vector<FrameGraphResourceHandle> handles;
	};

	struct CompiledPlan
	{
		std::vector<CompilePassRecord> passes;
		std::vector<CompileResourceEntry> resources;
		std::vector<CompiledTransientResourcePlan> transientResources;
		std::vector<CompiledPhysicalBlockPlan> physicalBlocks;
		std::vector<PassIndex> executionOrder;
		std::vector<CompiledAliasingBarrier> finalAliasingBarriers;
		std::vector<CompiledBarrier> finalBarriers;

		void Clear() noexcept
		{
			passes.clear();
			resources.clear();
			transientResources.clear();
			physicalBlocks.clear();
			executionOrder.clear();
			finalAliasingBarriers.clear();
			finalBarriers.clear();
		}
	};

  private:
	friend class PassBuilder;

	using SetupCallback = std::function<void(PassBuilder&, const FrameContext&)>;
	using ExecuteCallback = std::function<void(RenderGraphPassContext&)>;

	template <typename TPass, typename TParameterBindings, typename... TExecuteArgs>
	static auto MakeDirectPassExecuteCallback(TExecuteArgs&&... executeArgs)
	{
		using ExecuteArgsTuple = std::tuple<std::decay_t<TExecuteArgs>...>;
		return [executeArgsTuple = ExecuteArgsTuple(std::forward<TExecuteArgs>(executeArgs)...)](
		           RenderGraphPassContext& context,
		           TParameterBindings& typedParameters) mutable
		{
			std::apply(
			    [&](auto&... capturedExecuteArgs)
			    {
				    TPass::Execute(context, typedParameters, capturedExecuteArgs...);
			    },
			    executeArgsTuple);
		};
	}

	template <typename TParameterBindings, typename ExecuteFn>
	static ExecuteCallback MakeParameterizedExecuteCallback(TParameterBindings* parameters, ExecuteFn&& executeFn)
	{
		using ExecuteFnType = std::decay_t<ExecuteFn>;
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, RenderGraphPassContext&, TParameterBindings&>,
		    "Typed pass execute lambda must accept (RenderGraphPassContext&, Parameters&). ");

		ExecuteFnType callback(std::forward<ExecuteFn>(executeFn));
		return [parameters, callback = std::move(callback)](RenderGraphPassContext& context) mutable
		{
			callback(context, *parameters);
		};
	}

	template <typename TParameterBindings, typename SetupFn, typename ExecuteFn>
	void AddTypedShaderPass(
	    std::string_view name,
	    EFrameGraphPassFlags flags,
	    TParameterBindings& parameters,
	    SetupFn&& setupFn,
	    ExecuteFn&& executeFn)
	{
		auto* parameterBindings = &parameters;
		auto setupValid = std::make_shared<bool>(true);
		std::string passName(name);

		m_passes.push_back(
		    RegisteredPass{
		        std::string(name),
		        flags,
		        [parameterBindings, passName, setupValid, setupFn = std::forward<SetupFn>(setupFn)](
		            PassBuilder& builder,
		            const FrameContext&) mutable
		        {
			        *setupValid = setupFn(builder, *parameterBindings, passName.c_str());
		        },
		        MakeParameterizedExecuteCallback(
		            parameterBindings,
		            [setupValid, executeFn = std::forward<ExecuteFn>(executeFn)](
		                RenderGraphPassContext& context,
		                TParameterBindings& typedParameters) mutable
		            {
			            if (!*setupValid)
			            {
				            return;
			            }

			            executeFn(context, typedParameters);
		            })});
	}

	void BeginPassSetup() noexcept;
	void EndPassSetup() noexcept;
	void RecordDeclaration(PassResourceDeclaration declaration) noexcept;
	FrameGraphResourceHandle Read(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept;
	FrameGraphResourceHandle Write(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept;
	FrameGraphResourceHandle Use(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept;
	FrameGraphResourceHandle Read(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphResourceHandle Write(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphResourceHandle Use(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;

	RhiCpuDescriptorHandle ResolveRenderTargetView(FrameGraphResourceHandle handle) const noexcept;
	RhiCpuDescriptorHandle ResolveDepthStencilView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept;
	RhiCpuDescriptorHandle ResolveTransientRenderTargetView(FrameGraphResourceHandle handle) const noexcept;
	RhiCpuDescriptorHandle ResolveTransientDepthStencilView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveTransientShaderResourceView(FrameGraphResourceHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveTransientUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept;
	std::array<float, 4> GetClearColor(FrameGraphResourceHandle handle) const noexcept;
	float GetClearDepth(FrameGraphResourceHandle handle) const noexcept;
	NativeResourceHandle ResolveResource(FrameGraphResourceHandle handle) const noexcept;
	NativeResourceHandle ResolveTransientResource(FrameGraphResourceHandle handle, FrameGraphResourceKind kind) const noexcept;
	void CopyResource(RenderCommandContext& cmd, FrameGraphResourceHandle destinationHandle, FrameGraphResourceHandle sourceHandle) const noexcept;
	void SyncImportedResourceAccesses() const noexcept;
	void BuildTransientMaterializationPlan(CompiledPlan& plan) const noexcept;
	void EnsureTransientResourcesMaterialized(const CompiledPlan& plan) const noexcept;
	void ReleaseExternalViewDescriptors() noexcept;
	void EmitCompiledAliasingBarriers(RenderCommandContext& cmd, const std::vector<CompiledAliasingBarrier>& barriers) const noexcept;
	void EmitCompiledAliasingBarriers(RenderCommandContext& cmd, std::string_view passName, const std::vector<CompiledAliasingBarrier>& barriers)
	    const noexcept;
	void EmitCompiledBarriers(RenderCommandContext& cmd, const std::vector<CompiledBarrier>& barriers) const noexcept;
	void EmitCompiledBarriers(RenderCommandContext& cmd, std::string_view passName, const std::vector<CompiledBarrier>& barriers) const noexcept;
	FrameGraphResourceHandle AllocateDynamicResourceHandle() noexcept;

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
		EFrameGraphPassFlags flags = EFrameGraphPassFlags::None;
		SetupCallback setupCallback;
		ExecuteCallback executeCallback;
	};

	std::vector<RegisteredPass> m_passes;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	Window* m_window = nullptr;
	RenderViewportExtent m_sceneExtent = {};
	mutable ResourceRegistry m_resourceRegistry;
	CompiledPlan m_compiledPlan;
	std::uint32_t m_nextDynamicResourceIndex = 0;
	std::vector<VirtualTransientResource> m_virtualTransientResources;
	mutable std::unique_ptr<FrameGraphTransientAllocator> m_transientAllocator;
	std::vector<std::unique_ptr<AllocatedParameterInstanceBase>> m_allocatedParameterInstances;
	std::vector<PassResourceDeclaration> m_activePassDeclarations;
	bool m_isSettingUpPass = false;
};
