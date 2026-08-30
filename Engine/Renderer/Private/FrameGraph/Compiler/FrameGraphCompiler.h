#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "RHI/Public/Commands/RhiQueueCapabilities.h"

#include <vector>

class FrameGraphResourceRegistry;
class FrameGraphResourceStateTracker;

class FrameGraphCompiler final
{
public:
	FrameGraphCompiler(
	    FrameGraphPlan& plan,
	    FrameGraphResourceRegistry& resourceRegistry,
	    FrameGraphResourceStateTracker& resourceStateTracker,
	    const RhiQueueCapabilities& queueCapabilities) noexcept;

	void Compile() noexcept;

private:
	struct LastResourceAccess final
	{
		FrameGraphPassIndex Pass = INVALID_FRAME_GRAPH_PASS_INDEX;
		ERhiQueueType Queue = ERhiQueueType::Graphics;
	};

	using LastResourceAccessTable = std::vector<LastResourceAccess>;

	void BuildCompiledPlanResources() noexcept;
	void BuildResourceVersionGraph() noexcept;
	void FinalizePassDependencies() noexcept;
	void DeduplicatePassDependencies() noexcept;
	void CullDeadPasses() noexcept;
	void MarkPassAliveRecursive(FrameGraphPassIndex passIndex) noexcept;
	const char* GetRootPassReason(const FrameGraphPassNode& passRecord) const noexcept;
	bool WritesBackBuffer(const FrameGraphPassNode& passRecord) const noexcept;
	bool WritesProductRoot(const FrameGraphPassNode& passRecord) const noexcept;
	void BuildPassSuccessorsAndInDegrees() noexcept;
	void BuildTopologicalExecutionOrder() noexcept;
	void ValidateExecutionOrder() const noexcept;
	void AssignPassQueues() noexcept;
	void BuildSubmissionBatches() noexcept;
	void AddSynchronizationDependency(FrameGraphPassNode& passRecord, FrameGraphPassIndex dependency) noexcept;
	void BuildTransientResourceLifetimes() noexcept;
	void BuildTransientPhysicalBlockAssignments() noexcept;
	void BuildTransientAliasingBarriers() noexcept;
	void BuildResourceBarriers() noexcept;
	void InitializeBarrierDependencies() noexcept;
	void BuildPassResourceBarriers(FrameGraphPassIndex passIndex, LastResourceAccessTable& lastResourceAccesses) noexcept;
	void BuildResourceBarrier(
	    FrameGraphPassIndex passIndex,
	    FrameGraphPassNode& passRecord,
	    const PassResourceDeclaration& declaration,
	    FrameGraphResourceNode& compiledResource,
	    LastResourceAccess& lastAccess) noexcept;
	void BuildQueueOwnershipBarriers(
	    FrameGraphPassIndex passIndex,
	    FrameGraphPassNode& passRecord,
	    const PassResourceDeclaration& declaration,
	    FrameGraphResourceNode& compiledResource,
	    const LastResourceAccess& lastAccess) noexcept;
	void BuildRequiredResourceBarriers(
	    FrameGraphPassNode& passRecord,
	    const PassResourceDeclaration& declaration,
	    FrameGraphResourceNode& compiledResource) noexcept;
	void BuildFinalResourceBarriers(const LastResourceAccessTable& lastResourceAccesses) noexcept;
	static bool HasCompiledBarrier(
	    const FrameGraphPassNode& passRecord,
	    FrameGraphResourceHandle handle,
	    FrameGraphBarrier::Type type) noexcept;
	void ResetCompiledResourceStatesForBarrierPlanning() noexcept;
	ResourceState InferRequiredResourceState(
	    const PassResourceDeclaration& declaration,
	    const FrameGraphResourceNode& resource) const noexcept;
	bool ShouldRestoreFinalState(const FrameGraphResourceNode& resource) const noexcept;
	void BuildPassResourceVersionDependencies(FrameGraphPassNode& passRecord) noexcept;
	void RegisterReadDependency(FrameGraphPassNode& passRecord, FrameGraphResourceNode& resource) noexcept;
	void RegisterWriteDependency(FrameGraphPassNode& passRecord, FrameGraphResourceNode& resource) noexcept;
	FrameGraphResourceVersion& GetCurrentResourceVersion(FrameGraphResourceNode& resource) noexcept;
	const FrameGraphResourceVersion& GetCurrentResourceVersion(const FrameGraphResourceNode& resource) const noexcept;
	FrameGraphResourceNode& GetCompiledResourceEntry(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceNode& GetCompiledResourceEntry(FrameGraphResourceHandle handle) const noexcept;
	FrameGraphTransientResourcePlan* FindTransientResourcePlan(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphTransientResourcePlan* FindTransientResourcePlan(FrameGraphResourceHandle handle) const noexcept;

	FrameGraphPlan& m_plan;
	FrameGraphResourceRegistry& m_resourceRegistry;
	FrameGraphResourceStateTracker& m_resourceStateTracker;
	RhiQueueCapabilities m_queueCapabilities;
};
