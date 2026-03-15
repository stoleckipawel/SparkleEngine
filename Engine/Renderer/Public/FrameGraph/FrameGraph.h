#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/FrameGraphPassFlags.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/PassResourceDeclaration.h"
#include "Renderer/Public/FrameGraph/PassBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/BufferHandle.h"
#include "Renderer/Public/FrameGraph/ResourceRegistry.h"
#include "Renderer/Public/FrameGraph/ResourceHandle.h"
#include "Renderer/Public/FrameGraph/TextureHandle.h"

#include <d3d12.h>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12SwapChain;
class CommandContext;
class D3D12Texture;
class FrameGraphTransientAllocator;
class Window;
struct FrameContext;

class SPARKLE_RENDERER_API FrameGraph
{
  public:
	struct CompiledPlan;

	FrameGraph(D3D12Rhi* rhi, Window* window, D3D12DescriptorHeapManager* descriptorHeapManager, D3D12SwapChain* swapChain);
	~FrameGraph();

	FrameGraph(const FrameGraph&) = delete;
	FrameGraph& operator=(const FrameGraph&) = delete;
	FrameGraph(FrameGraph&&) = delete;
	FrameGraph& operator=(FrameGraph&&) = delete;

	template <typename SetupFn, typename ExecuteFn>
	void AddPass(std::string_view name, FrameGraphPassFlags flags, SetupFn&& setupFn, ExecuteFn&& executeFn)
	{
		AddLambdaPass(
		    name,
		    flags,
		    MakeSetupCallback(std::forward<SetupFn>(setupFn)),
		    MakeExecuteCallback(std::forward<ExecuteFn>(executeFn)));
	}

	void Setup(const FrameContext& frame);

	CompiledPlan Compile();

	void Execute(const CompiledPlan& plan, CommandContext& cmd, const FrameContext& frame) const;
	TextureHandle ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	TextureHandle ImportTexture(const FrameGraphTextureDesc& desc, ID3D12Resource& resource, ResourceState initialState) noexcept;
	TextureHandle ImportTexture(const FrameGraphTextureDesc& desc, D3D12Texture& texture, ResourceState initialState) noexcept;
	TextureHandle CreateTexture(const FrameGraphTextureDesc& desc) noexcept;
	BufferHandle ImportBuffer(const FrameGraphBufferDesc& desc, ID3D12Resource& resource, ResourceState initialState) noexcept;
	BufferHandle CreateBuffer(const FrameGraphBufferDesc& desc) noexcept;
	void BindRenderTarget(
	    CommandContext& cmd,
	    TextureHandle renderTargetHandle,
	    TextureHandle depthStencilHandle = TextureHandle::Invalid()) const noexcept;
	void CopyTexture(CommandContext& cmd, TextureHandle destinationHandle, TextureHandle sourceHandle) const noexcept;
	void CopyBuffer(CommandContext& cmd, BufferHandle destinationHandle, BufferHandle sourceHandle) const noexcept;
	void ClearRenderTarget(CommandContext& cmd, TextureHandle handle) const noexcept;
	void ClearDepthStencil(CommandContext& cmd, TextureHandle handle) const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveShaderResourceView(TextureHandle handle) const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveShaderResourceView(BufferHandle handle) const noexcept;

	using PassIndex = std::uint32_t;
	using ResourceIndex = std::uint32_t;
	static constexpr PassIndex INVALID_PASS_INDEX = static_cast<PassIndex>(-1);
	static constexpr ResourceIndex INVALID_RESOURCE_INDEX = static_cast<ResourceIndex>(-1);

	struct CompiledBarrier
	{
		ResourceHandle handle = ResourceHandle::Invalid();
		ResourceState before = ResourceState::Common;
		ResourceState after = ResourceState::Common;
	};

	struct CompiledAliasingBarrier
	{
		std::uint32_t physicalBlockIndex = INVALID_RESOURCE_INDEX;
		ResourceHandle beforeHandle = ResourceHandle::Invalid();
		ResourceHandle afterHandle = ResourceHandle::Invalid();
		PassIndex executeBeforePass = INVALID_PASS_INDEX;
		PassIndex executeAfterPass = INVALID_PASS_INDEX;
	};

	struct ResourceVersion
	{
		ResourceHandle handle = ResourceHandle::Invalid();
		std::uint32_t version = 0;
		PassIndex writerPass = INVALID_PASS_INDEX;
		std::vector<PassIndex> readerPasses;
	};

	struct CompilePassRecord
	{
		PassIndex index = INVALID_PASS_INDEX;
		std::string passName;
		FrameGraphPassFlags flags = FrameGraphPassFlags::None;
		FrameGraphPassFlags passKind = FrameGraphPassFlags::None;
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
		ResourceHandle handle = ResourceHandle::Invalid();
		FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
		FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
		FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
		ResourceState initialState = ResourceState::Common;
		ResourceState finalState = ResourceState::Common;
		ResourceState currentState = ResourceState::Common;
		std::string debugName;
		std::string displayLabel;
		std::string eventScopeLabel;
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
			UINT64 sizeInBytes = 0;
			UINT64 alignment = 0;
			UINT64 heapOffset = 0;
			D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
			D3D12_RESOURCE_DESC resourceDesc{};
			D3D12_CLEAR_VALUE optimizedClearValue{};
			bool hasOptimizedClearValue = false;
			ResourceState initialState = ResourceState::Common;
		};

		ResourceHandle handle = ResourceHandle::Invalid();
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
		std::string displayLabel;
		std::string eventScopeLabel;
		std::vector<ResourceState> requiredStates;
	};

	struct CompiledPhysicalBlockPlan
	{
		std::uint32_t physicalBlockIndex = INVALID_RESOURCE_INDEX;
		CompiledTransientResourcePlan::AllocationPool pool = CompiledTransientResourcePlan::AllocationPool::Color;
		UINT64 sizeInBytes = 0;
		UINT64 alignment = 0;
		UINT64 heapOffset = 0;
		D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
		D3D12_RESOURCE_DESC resourceDesc{};
		D3D12_CLEAR_VALUE optimizedClearValue{};
		bool hasOptimizedClearValue = false;
		PassIndex firstExecutionIndex = INVALID_PASS_INDEX;
		PassIndex lastExecutionIndex = INVALID_PASS_INDEX;
		std::string displayLabel;
		std::string eventScopeLabel;
		std::vector<ResourceHandle> handles;
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
	using ExecuteCallback = std::function<void(const FrameGraph&, CommandContext&, const FrameContext&)>;

	template <typename SetupFn> static SetupCallback MakeSetupCallback(SetupFn&& setupFn)
	{
		using SetupFnType = std::decay_t<SetupFn>;
		static_assert(
		    std::is_invocable_v<SetupFnType&, PassBuilder&, const FrameContext&> ||
		        std::is_invocable_v<SetupFnType&, PassBuilder&>,
		    "FrameGraph setup lambda must accept (PassBuilder&, const FrameContext&) or (PassBuilder&).\n");

		SetupFnType callback(std::forward<SetupFn>(setupFn));
		if constexpr (std::is_invocable_v<SetupFnType&, PassBuilder&, const FrameContext&>)
		{
			return [callback = std::move(callback)](PassBuilder& builder, const FrameContext& frame) mutable
			{
				callback(builder, frame);
			};
		}
		else
		{
			return [callback = std::move(callback)](PassBuilder& builder, const FrameContext&) mutable
			{
				callback(builder);
			};
		}
	}

	template <typename ExecuteFn> static ExecuteCallback MakeExecuteCallback(ExecuteFn&& executeFn)
	{
		using ExecuteFnType = std::decay_t<ExecuteFn>;
		static_assert(
		    std::is_invocable_v<ExecuteFnType&, const FrameGraph&, CommandContext&, const FrameContext&> ||
		        std::is_invocable_v<ExecuteFnType&, CommandContext&, const FrameContext&> ||
		        std::is_invocable_v<ExecuteFnType&, CommandContext&> ||
		        std::is_invocable_v<ExecuteFnType&, const FrameContext&> ||
		        std::is_invocable_v<ExecuteFnType&>,
		    "FrameGraph execute lambda must accept (const FrameGraph&, CommandContext&, const FrameContext&), (CommandContext&, const FrameContext&), (CommandContext&), (const FrameContext&), or ().");

		ExecuteFnType callback(std::forward<ExecuteFn>(executeFn));
		if constexpr (std::is_invocable_v<ExecuteFnType&, const FrameGraph&, CommandContext&, const FrameContext&>)
		{
			return [callback = std::move(callback)](const FrameGraph& frameGraph, CommandContext& cmd, const FrameContext& frame) mutable
			{
				callback(frameGraph, cmd, frame);
			};
		}
		else if constexpr (std::is_invocable_v<ExecuteFnType&, CommandContext&, const FrameContext&>)
		{
			return [callback = std::move(callback)](const FrameGraph&, CommandContext& cmd, const FrameContext& frame) mutable
			{
				callback(cmd, frame);
			};
		}
		else if constexpr (std::is_invocable_v<ExecuteFnType&, CommandContext&>)
		{
			return [callback = std::move(callback)](const FrameGraph&, CommandContext& cmd, const FrameContext&) mutable
			{
				callback(cmd);
			};
		}
		else if constexpr (std::is_invocable_v<ExecuteFnType&, const FrameContext&>)
		{
			return [callback = std::move(callback)](const FrameGraph&, CommandContext&, const FrameContext& frame) mutable
			{
				callback(frame);
			};
		}
		else
		{
			return [callback = std::move(callback)](const FrameGraph&, CommandContext&, const FrameContext&) mutable
			{
				callback();
			};
		}
	}

	void AddLambdaPass(std::string_view name, FrameGraphPassFlags flags, SetupCallback setupCallback, ExecuteCallback executeCallback);
	void BeginPassSetup() noexcept;
	void EndPassSetup() noexcept;
	void RecordDeclaration(PassResourceDeclaration declaration) noexcept;
	ResourceHandle Read(ResourceHandle handle, ResourceUsage usage) noexcept;
	ResourceHandle Write(ResourceHandle handle, ResourceUsage usage) noexcept;

	D3D12_CPU_DESCRIPTOR_HANDLE ResolveRenderTargetView(ResourceHandle handle) const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE ResolveDepthStencilView(ResourceHandle handle) const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveShaderResourceView(ResourceHandle handle) const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE ResolveTransientRenderTargetView(ResourceHandle handle) const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE ResolveTransientDepthStencilView(ResourceHandle handle) const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveTransientShaderResourceView(ResourceHandle handle) const noexcept;
	std::array<float, 4> GetClearColor(ResourceHandle handle) const noexcept;
	float GetClearDepth(ResourceHandle handle) const noexcept;
	ID3D12Resource* ResolveResource(ResourceHandle handle) const noexcept;
	ID3D12Resource* ResolveTransientResource(ResourceHandle handle, FrameGraphResourceKind kind) const noexcept;
	void CopyResource(CommandContext& cmd, ResourceHandle destinationHandle, ResourceHandle sourceHandle) const noexcept;
	void SyncImportedResourceAccesses() const noexcept;
	void BuildTransientMaterializationPlan(CompiledPlan& plan) const noexcept;
	void EnsureTransientResourcesMaterialized(const CompiledPlan& plan) const noexcept;
	void ReleaseExternalViewDescriptors() noexcept;
	void EmitCompiledAliasingBarriers(CommandContext& cmd, const std::vector<CompiledAliasingBarrier>& barriers) const noexcept;
	void EmitCompiledAliasingBarriers(CommandContext& cmd, std::string_view passName, const std::vector<CompiledAliasingBarrier>& barriers)
	    const noexcept;
	void EmitCompiledBarriers(CommandContext& cmd, const std::vector<CompiledBarrier>& barriers) const noexcept;
	void EmitCompiledBarriers(CommandContext& cmd, std::string_view passName, const std::vector<CompiledBarrier>& barriers) const noexcept;
	ResourceHandle AllocateDynamicResourceHandle() noexcept;

	struct VirtualTransientResource
	{
		ResourceHandle handle;
		FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
		FrameGraphTextureDesc textureDesc{};
		FrameGraphBufferDesc bufferDesc{};
	};

	struct RegisteredPass
	{
		std::string name;
		FrameGraphPassFlags flags = FrameGraphPassFlags::None;
		SetupCallback setupCallback;
		ExecuteCallback executeCallback;
	};

	std::vector<RegisteredPass> m_passes;
	D3D12Rhi* m_rhi = nullptr;
	Window* m_window = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	mutable ResourceRegistry m_resourceRegistry;
	CompiledPlan m_compiledPlan;
	std::uint32_t m_nextDynamicResourceIndex = 0;
	std::vector<VirtualTransientResource> m_virtualTransientResources;
	mutable std::unique_ptr<FrameGraphTransientAllocator> m_transientAllocator;
	std::vector<PassResourceDeclaration> m_activePassDeclarations;
	bool m_isSettingUpPass = false;
};
