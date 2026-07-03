#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"

class FrameGraphResourceRegistry;
class FrameGraphResourceStateTracker;

class FrameGraphCompiler final
{
  public:
	FrameGraphCompiler(
	    FrameGraphPlan& plan,
	    FrameGraphResourceRegistry& resourceRegistry,
	    FrameGraphResourceStateTracker& resourceStateTracker) noexcept;

	void Compile() noexcept;

  private:
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
	void BuildTransientResourceLifetimes() noexcept;
	void BuildTransientPhysicalBlockAssignments() noexcept;
	void BuildTransientAliasingBarriers() noexcept;
	void ResetCompiledResourceStatesForBarrierPlanning() noexcept;
	ResourceState InferRequiredResourceState(const PassResourceDeclaration& declaration, const FrameGraphResourceNode& resource)
	    const noexcept;
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
};
