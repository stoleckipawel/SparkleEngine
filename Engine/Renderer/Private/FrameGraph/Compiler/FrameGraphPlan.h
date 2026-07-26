#pragma once

#include "FrameGraph/FrameGraphPassKind.h"
#include "FrameGraph/FrameGraphQueuePreference.h"
#include "FrameGraph/Compiler/FrameGraphRecordingPlan.h"
#include "FrameGraph/FrameGraphResourceTypes.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <cstdint>
#include <string>
#include <vector>

using FrameGraphPassIndex = std::uint32_t;
using FrameGraphResourceIndex = std::uint32_t;

static constexpr FrameGraphPassIndex INVALID_FRAME_GRAPH_PASS_INDEX = static_cast<FrameGraphPassIndex>(-1);
static constexpr FrameGraphResourceIndex INVALID_FRAME_GRAPH_RESOURCE_INDEX = static_cast<FrameGraphResourceIndex>(-1);

enum class FrameGraphPassExecutionModel : std::uint8_t
{
	Callback,
	TypedShader,
};

struct FrameGraphBarrier
{
	enum class Type : std::uint8_t
	{
		Transition,
		UnorderedAccess,
		AccelerationStructure
	};

	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	Type type = Type::Transition;
	ResourceState before = ResourceState::Common;
	ResourceState after = ResourceState::Common;
	std::string label;
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

struct FrameGraphProductRoot
{
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	std::string name;
	ResourceState requiredState = ResourceState::ShaderResource;
};

struct FrameGraphPassNode
{
	FrameGraphPassIndex index = INVALID_FRAME_GRAPH_PASS_INDEX;
	std::string passName;
	EFrameGraphPassKind kind = EFrameGraphPassKind::None;
	EFrameGraphQueuePreference queuePreference = EFrameGraphQueuePreference::Graphics;
	ERhiQueueType queue = ERhiQueueType::Graphics;
	std::string diagnosticName;
	std::string eventScopeLabel;
	std::vector<PassResourceDeclaration> declarations;
	std::vector<FrameGraphPassIndex> dependsOn;
	std::vector<FrameGraphPassIndex> synchronizationDependencies;
	std::vector<FrameGraphPassIndex> successors;
	std::uint32_t inDegree = 0;
	bool alive = true;
	std::vector<FrameGraphAliasingBarrier> transientAliasingBarriers;
	std::vector<FrameGraphBarrier> compiledBarriers;
	std::vector<FrameGraphBarrier> compiledReleaseBarriers;
	FrameGraphPassExecutionModel executionModel = FrameGraphPassExecutionModel::Callback;
};

using FrameGraphSubmissionBatchIndex = std::uint32_t;
static constexpr FrameGraphSubmissionBatchIndex INVALID_FRAME_GRAPH_SUBMISSION_BATCH_INDEX =
    static_cast<FrameGraphSubmissionBatchIndex>(-1);

struct FrameGraphSubmissionBatch final
{
	FrameGraphSubmissionBatchIndex index = INVALID_FRAME_GRAPH_SUBMISSION_BATCH_INDEX;
	ERhiQueueType queue = ERhiQueueType::Graphics;
	std::vector<FrameGraphPassIndex> passes;
	std::vector<FrameGraphSubmissionBatchIndex> waitForBatches;
	RecordingChunkIndex recordingChunkOffset = InvalidRecordingChunkIndex;
	std::uint32_t recordingChunkCount = 0;
};

struct FrameGraphResourceNode
{
	FrameGraphResourceIndex index = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
	FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
	FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
	ResourceState initialState = ResourceState::Common;
	ResourceState planningStartState = ResourceState::Common;
	ResourceState finalState = ResourceState::Common;
	ResourceState currentState = ResourceState::Common;
	std::string debugName;
	bool pendingAccelerationStructureBarrier = false;
	std::uint32_t currentVersion = 0;
	std::vector<FrameGraphResourceVersion> versions;
};

struct FrameGraphTransientLifetime
{
	FrameGraphPassIndex firstUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex lastUserPass = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex firstExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	FrameGraphPassIndex lastExecutionIndex = INVALID_FRAME_GRAPH_PASS_INDEX;
	bool readUsed = false;
	bool writeUsed = false;
	std::vector<ResourceState> requiredStates;

	void Clear() noexcept;
};

struct FrameGraphTransientResourcePlan
{
	struct PhysicalAllocationPlan
	{
		std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		RhiTransientAllocationPool pool = RhiTransientAllocationPool::Texture;
		std::uint64_t sizeInBytes = 0;
		std::uint64_t alignment = 0;
		std::uint64_t memoryBlockOffset = 0;
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
	FrameGraphTransientLifetime lifetime{};
};

struct FrameGraphTransientPhysicalBlockPlan
{
	std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
	std::vector<FrameGraphResourceHandle> handles;
};

struct FrameGraphTransientPlan
{
	std::vector<FrameGraphTransientResourcePlan> resources;
	std::vector<FrameGraphTransientPhysicalBlockPlan> physicalBlocks;

	void Clear() noexcept;
};

struct FrameGraphPlan
{
	std::vector<FrameGraphPassNode> passes;
	std::vector<FrameGraphResourceNode> resources;
	std::vector<FrameGraphProductRoot> productRoots;
	FrameGraphTransientPlan transients;
	std::vector<FrameGraphPassIndex> executionOrder;
	std::vector<FrameGraphAliasingBarrier> initialTransientAliasingBarriers;
	std::vector<FrameGraphBarrier> initialBarriers;
	std::vector<FrameGraphBarrier> finalBarriers;
	std::vector<FrameGraphSubmissionBatch> submissionBatches;
	RecordingPlan recording;

	void Clear() noexcept;
};
