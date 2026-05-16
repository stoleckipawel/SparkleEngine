#pragma once

#include "FrameGraph/FrameGraphPassFlags.h"
#include "FrameGraph/FrameGraphResourceTypes.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>
#include <string>
#include <vector>

using FrameGraphPassIndex = std::uint32_t;
using FrameGraphResourceIndex = std::uint32_t;

static constexpr FrameGraphPassIndex INVALID_FRAME_GRAPH_PASS_INDEX = static_cast<FrameGraphPassIndex>(-1);
static constexpr FrameGraphResourceIndex INVALID_FRAME_GRAPH_RESOURCE_INDEX = static_cast<FrameGraphResourceIndex>(-1);

struct FrameGraphBarrier
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

struct FrameGraphAliasingBarrier
{
	std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
	FrameGraphResourceHandle beforeHandle = FrameGraphResourceHandle::Invalid();
	FrameGraphResourceHandle afterHandle = FrameGraphResourceHandle::Invalid();
	FrameGraphPassIndex executeBeforePass = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex executeAfterPass = INVALID_FRAME_GRAPH_PASS_INDEX;
};

struct FrameGraphResourceVersion
{
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	std::uint32_t version = 0;
	FrameGraphPassIndex writerPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	std::vector<FrameGraphPassIndex> readerPasses;
};

struct FrameGraphPassNode
{
	FrameGraphPassIndex index = INVALID_FRAME_GRAPH_PASS_INDEX;
	std::string passName;
	EFrameGraphPassFlags flags = EFrameGraphPassFlags::None;
	EFrameGraphPassFlags passKind = EFrameGraphPassFlags::None;
	std::string diagnosticName;
	std::string displayLabel;
	std::string eventScopeLabel;
	std::vector<PassResourceDeclaration> declarations;
	std::vector<FrameGraphPassIndex> dependsOn;
	std::vector<FrameGraphPassIndex> successors;
	std::uint32_t inDegree = 0;
	bool alive = true;
	std::vector<FrameGraphAliasingBarrier> compiledAliasingBarriers;
	std::vector<FrameGraphBarrier> compiledBarriers;
};

struct FrameGraphResourceNode
{
	FrameGraphResourceIndex index = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
	FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
	FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
	ResourceState initialState = ResourceState::Common;
	ResourceState finalState = ResourceState::Common;
	ResourceState currentState = ResourceState::Common;
	std::string debugName;
	std::uint32_t currentVersion = 0;
	std::vector<FrameGraphResourceVersion> versions;
};

struct FrameGraphTransientResourcePlan
{
	enum class AllocationPool : std::uint8_t
	{
		Color,
		Depth,
		Buffer
	};

	struct PhysicalAllocationPlan
	{
		std::uint32_t allocationIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
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
	FrameGraphPassIndex firstUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex lastUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex firstExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex lastExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	bool readUsed = false;
	bool writeUsed = false;
	std::vector<ResourceState> requiredStates;
};

struct FrameGraphPhysicalAllocationPlan
{
	std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
	FrameGraphTransientResourcePlan::AllocationPool pool = FrameGraphTransientResourcePlan::AllocationPool::Color;
	std::uint64_t sizeInBytes = 0;
	std::uint64_t alignment = 0;
	std::uint64_t heapOffset = 0;
	RhiTextureResourceDesc textureResourceDesc{};
	RhiBufferResourceDesc bufferResourceDesc{};
	RhiOptimizedClearValue optimizedClearValue{};
	bool hasOptimizedClearValue = false;
	FrameGraphPassIndex firstExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex lastExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	std::vector<FrameGraphResourceHandle> handles;
};

struct FrameGraphPlan
{
	std::vector<FrameGraphPassNode> passes;
	std::vector<FrameGraphResourceNode> resources;
	std::vector<FrameGraphTransientResourcePlan> transientResources;
	std::vector<FrameGraphPhysicalAllocationPlan> physicalBlocks;
	std::vector<FrameGraphPassIndex> executionOrder;
	std::vector<FrameGraphAliasingBarrier> finalAliasingBarriers;
	std::vector<FrameGraphBarrier> finalBarriers;

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
