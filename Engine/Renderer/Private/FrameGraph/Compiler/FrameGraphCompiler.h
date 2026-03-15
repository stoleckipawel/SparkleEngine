#pragma once

#include "Renderer/Public/FrameGraph/FrameGraph.h"

class ResourceRegistry;

class FrameGraphCompiler final {
  public:
    FrameGraphCompiler(FrameGraph::CompiledPlan& plan, ResourceRegistry& resourceRegistry) noexcept;

    void Compile() noexcept;

  private:
    using PassIndex = FrameGraph::PassIndex;
    using ResourceIndex = FrameGraph::ResourceIndex;
    using CompiledBarrier = FrameGraph::CompiledBarrier;
    using ResourceVersion = FrameGraph::ResourceVersion;
    using CompilePassRecord = FrameGraph::CompilePassRecord;
    using CompileResourceEntry = FrameGraph::CompileResourceEntry;

    void BuildCompiledPlanResources() noexcept;
    void BuildResourceVersionGraph() noexcept;
    void FinalizePassDependencies() noexcept;
    void DeduplicatePassDependencies() noexcept;
    void CullDeadPasses() noexcept;
    void MarkPassAliveRecursive(PassIndex passIndex) noexcept;
    bool IsRootPass(const CompilePassRecord& passRecord) const noexcept;
    const char* GetRootPassReason(const CompilePassRecord& passRecord) const noexcept;
    bool WritesBackBuffer(const CompilePassRecord& passRecord) const noexcept;
    void BuildPassSuccessorsAndInDegrees() noexcept;
    void BuildTopologicalExecutionOrder() noexcept;
    void ValidateExecutionOrder() const noexcept;
    void BuildTransientResourceLifetimes() noexcept;
    void BuildTransientPhysicalBlockAssignments() noexcept;
    void BuildTransientAliasingBarriers() noexcept;
    void ResetCompiledResourceStatesForBarrierPlanning() noexcept;
    ResourceState InferRequiredResourceState(const PassResourceDeclaration& declaration,
                                            const CompileResourceEntry& resource) const noexcept;
    bool ShouldRestoreFinalState(const CompileResourceEntry& resource) const noexcept;
    void BuildPassResourceVersionDependencies(CompilePassRecord& passRecord) noexcept;
    void RegisterReadDependency(CompilePassRecord& passRecord, CompileResourceEntry& resource) noexcept;
    void RegisterWriteDependency(CompilePassRecord& passRecord, CompileResourceEntry& resource) noexcept;
    ResourceVersion& GetCurrentResourceVersion(CompileResourceEntry& resource) noexcept;
    const ResourceVersion& GetCurrentResourceVersion(const CompileResourceEntry& resource) const noexcept;
    CompileResourceEntry& GetCompiledResourceEntry(ResourceHandle handle) noexcept;
    const CompileResourceEntry& GetCompiledResourceEntry(ResourceHandle handle) const noexcept;
    FrameGraph::CompiledTransientResourcePlan* FindTransientResourcePlan(ResourceHandle handle) noexcept;
    const FrameGraph::CompiledTransientResourcePlan* FindTransientResourcePlan(ResourceHandle handle) const noexcept;

    FrameGraph::CompiledPlan& m_plan;
    ResourceRegistry& m_resourceRegistry;
};
