#include "PCH.h"
#include "FrameGraphCompiler.h"

#include "FrameGraph/Compiler/FrameGraphCompilerRayTracing.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"

#include <algorithm>
#include <cassert>

namespace
{
	void ValidateResourceVersionGraph(const FrameGraphPlan& plan) noexcept
	{
		for (const FrameGraphResourceNode& resource : plan.resources)
		{
			assert(resource.handle.IsValid());
			assert(!resource.versions.empty());
			assert(resource.currentVersion < resource.versions.size());

			for (std::size_t versionIndex = 0; versionIndex < resource.versions.size(); ++versionIndex)
			{
				const FrameGraphResourceVersion& version = resource.versions[versionIndex];
				assert(version.handle == resource.handle);
				assert(version.version == versionIndex);
				if (version.writerPass != INVALID_FRAME_GRAPH_PASS_INDEX)
				{
					assert(version.writerPass < plan.passes.size());
				}

				for (const FrameGraphPassIndex readerPass : version.readerPasses)
				{
					assert(readerPass < plan.passes.size());
				}
			}
		}
	}
}  // namespace

FrameGraphCompiler::FrameGraphCompiler(
	FrameGraphPlan& plan,
	FrameGraphResourceRegistry& resourceRegistry,
	FrameGraphResourceStateTracker& resourceStateTracker,
	const RhiQueueCapabilities& queueCapabilities) noexcept :
	m_plan(plan),
	m_resourceRegistry(resourceRegistry),
	m_resourceStateTracker(resourceStateTracker),
	m_queueCapabilities(queueCapabilities)
{
}

void FrameGraphCompiler::Compile() noexcept
{
	BuildCompiledPlanResources();
	m_plan.executionOrder.clear();
	m_plan.executionOrder.reserve(m_plan.passes.size());
	m_plan.initialBarriers.clear();
	m_plan.finalBarriers.clear();
	m_plan.submissionBatches.clear();

	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.dependsOn.clear();
		passRecord.successors.clear();
		passRecord.inDegree = 0;
		passRecord.alive = true;
		passRecord.compiledBarriers.clear();
		passRecord.compiledReleaseBarriers.clear();
		passRecord.synchronizationDependencies.clear();
	}

	BuildResourceVersionGraph();
	ValidateResourceVersionGraph(m_plan);
	FinalizePassDependencies();
	AssignPassQueues();
	BuildTransientResourceLifetimes();
	BuildTransientPhysicalBlockAssignments();
	BuildTransientAliasingBarriers();
	BuildResourceBarriers();
	BuildSubmissionBatches();
}

void FrameGraphCompiler::BuildCompiledPlanResources() noexcept
{
	m_plan.resources.clear();
	m_plan.resources.reserve(m_resourceRegistry.GetRegisteredHandles().size());

	const std::vector<FrameGraphResourceHandle>& registeredHandles = m_resourceRegistry.GetRegisteredHandles();
	for (std::size_t resourceIndex = 0; resourceIndex < registeredHandles.size(); ++resourceIndex)
	{
		const FrameGraphResourceHandle handle = registeredHandles[resourceIndex];
		assert(handle.IsValid());
		assert(handle.index == resourceIndex && "FrameGraph resources must be registered in monotonically increasing handle order.");
		const FrameGraphResourceMetadata& entry = m_resourceRegistry.GetMetadata(handle);
		const ResourceState runtimeState = m_resourceStateTracker.GetRuntimeState(handle);
		m_plan.resources.push_back(
		    FrameGraphResourceNode{
		        .index = static_cast<FrameGraphResourceIndex>(resourceIndex),
		        .handle = entry.handle,
		        .resourceClass = entry.resourceClass,
		        .kind = entry.kind,
		        .ownership = entry.ownership,
		        .initialState = entry.initialState,
		        .planningStartState = runtimeState,
		        .finalState = entry.finalState,
		        .currentState = runtimeState,
		        .debugName = entry.debugName,
		        .currentVersion = 0,
		        .versions = {FrameGraphResourceVersion{.handle = entry.handle, .version = 0, .writerPass = INVALID_FRAME_GRAPH_PASS_INDEX}}});
	}
}

void FrameGraphCompiler::ResetCompiledResourceStatesForBarrierPlanning() noexcept
{
	for (FrameGraphResourceNode& compiledResource : m_plan.resources)
	{
		compiledResource.currentState = m_resourceStateTracker.GetRuntimeState(compiledResource.handle);
		compiledResource.pendingAccelerationStructureBarrier = false;
		m_resourceStateTracker.UpdateCurrentState(compiledResource.handle, compiledResource.currentState);
	}
}

ResourceState FrameGraphCompiler::InferRequiredResourceState(
    const PassResourceDeclaration& declaration,
    const FrameGraphResourceNode& resource) const noexcept
{
	if (FrameGraphCompilerRayTracing::UsesRayTracingState(declaration))
	{
		return FrameGraphCompilerRayTracing::InferRequiredResourceState(declaration, resource);
	}

	if (IsReadOnlyUsage(declaration.usage))
	{
		switch (declaration.usage)
		{
			case ResourceUsage::DepthRead:
				assert(resource.kind == FrameGraphResourceKind::DepthStencil);
				return ResourceState::DepthRead;
			case ResourceUsage::ShaderRead:
				assert(resource.kind != FrameGraphResourceKind::BackBuffer);
				return ResourceState::ShaderResource;
			case ResourceUsage::CopySource:
				return ResourceState::CopySource;
			case ResourceUsage::Present:
				assert(resource.kind == FrameGraphResourceKind::BackBuffer);
				return ResourceState::Present;
			default:
				assert(false);
				return ResourceState::Common;
		}
	}

	if (IsWriteOnlyUsage(declaration.usage))
	{
		switch (declaration.usage)
		{
			case ResourceUsage::RenderTarget:
				assert(resource.kind == FrameGraphResourceKind::BackBuffer || resource.kind == FrameGraphResourceKind::ColorRenderTarget);
				return ResourceState::RenderTarget;
			case ResourceUsage::DepthWrite:
				assert(resource.kind == FrameGraphResourceKind::DepthStencil);
				return ResourceState::DepthWrite;
			case ResourceUsage::CopyDest:
				return ResourceState::CopyDest;
			default:
				assert(false);
				return ResourceState::Common;
		}
	}

	if (IsReadWriteUsage(declaration.usage))
	{
		assert(UsesUnorderedAccess(declaration.usage));
		assert(resource.kind != FrameGraphResourceKind::BackBuffer);
		assert(resource.kind != FrameGraphResourceKind::DepthStencil);
		return ResourceState::UnorderedAccess;
	}

	assert(false);
	return ResourceState::Common;
}

bool FrameGraphCompiler::ShouldRestoreFinalState(const FrameGraphResourceNode& resource) const noexcept
{
	return resource.finalState != ResourceState::Undefined &&
	       (resource.ownership != FrameGraphResourceOwnership::Transient || resource.kind == FrameGraphResourceKind::DepthStencil);
}

FrameGraphResourceVersion& FrameGraphCompiler::GetCurrentResourceVersion(FrameGraphResourceNode& resource) noexcept
{
	assert(resource.currentVersion < resource.versions.size());
	return resource.versions[resource.currentVersion];
}

const FrameGraphResourceVersion& FrameGraphCompiler::GetCurrentResourceVersion(const FrameGraphResourceNode& resource) const noexcept
{
	assert(resource.currentVersion < resource.versions.size());
	return resource.versions[resource.currentVersion];
}

FrameGraphResourceNode& FrameGraphCompiler::GetCompiledResourceEntry(FrameGraphResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	assert(handle.index < m_plan.resources.size());
	FrameGraphResourceNode& resource = m_plan.resources[handle.index];
	assert(resource.handle == handle);
	return resource;
}

const FrameGraphResourceNode& FrameGraphCompiler::GetCompiledResourceEntry(FrameGraphResourceHandle handle) const noexcept
{
	assert(handle.IsValid());
	assert(handle.index < m_plan.resources.size());
	const FrameGraphResourceNode& resource = m_plan.resources[handle.index];
	assert(resource.handle == handle);
	return resource;
}

FrameGraphTransientResourcePlan* FrameGraphCompiler::FindTransientResourcePlan(FrameGraphResourceHandle handle) noexcept
{
	const auto it = std::find_if(
	    m_plan.transients.resources.begin(),
	    m_plan.transients.resources.end(),
	    [handle](const FrameGraphTransientResourcePlan& transientPlan)
	    {
		    return transientPlan.handle == handle;
	    });

	return it != m_plan.transients.resources.end() ? &(*it) : nullptr;
}

const FrameGraphTransientResourcePlan* FrameGraphCompiler::FindTransientResourcePlan(FrameGraphResourceHandle handle) const noexcept
{
	const auto it = std::find_if(
	    m_plan.transients.resources.begin(),
	    m_plan.transients.resources.end(),
	    [handle](const FrameGraphTransientResourcePlan& transientPlan)
	    {
		    return transientPlan.handle == handle;
	    });

	return it != m_plan.transients.resources.end() ? &(*it) : nullptr;
}
