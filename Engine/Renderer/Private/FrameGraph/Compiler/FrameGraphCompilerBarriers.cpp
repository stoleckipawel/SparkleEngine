#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphCompiler.h"

#include "FrameGraph/Compiler/FrameGraphCompilerRayTracing.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"

#include <algorithm>

void FrameGraphCompiler::BuildResourceBarriers() noexcept
{
	ResetCompiledResourceStatesForBarrierPlanning();
	InitializeBarrierDependencies();

	LastResourceAccessTable lastResourceAccesses(m_plan.resources.size());
	for (const FrameGraphPassIndex passIndex : m_plan.executionOrder)
	{
		BuildPassResourceBarriers(passIndex, lastResourceAccesses);
	}

	BuildFinalResourceBarriers(lastResourceAccesses);
}

void FrameGraphCompiler::InitializeBarrierDependencies() noexcept
{
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.synchronizationDependencies = passRecord.dependsOn;
	}
}

void FrameGraphCompiler::BuildPassResourceBarriers(FrameGraphPassIndex passIndex, LastResourceAccessTable& lastResourceAccesses) noexcept
{
	FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
	for (const PassResourceDeclaration& declaration : passRecord.declarations)
	{
		if (!declaration.handle.IsValid())
		{
			continue;
		}

		FrameGraphResourceNode& compiledResource = GetCompiledResourceEntry(declaration.handle);
		LastResourceAccess& lastAccess = lastResourceAccesses[compiledResource.index];
		BuildResourceBarrier(passIndex, passRecord, declaration, compiledResource, lastAccess);
	}
}

void FrameGraphCompiler::BuildResourceBarrier(
    FrameGraphPassIndex passIndex,
    FrameGraphPassNode& passRecord,
    const PassResourceDeclaration& declaration,
    FrameGraphResourceNode& compiledResource,
    LastResourceAccess& lastAccess) noexcept
{
	BuildQueueOwnershipBarriers(passIndex, passRecord, declaration, compiledResource, lastAccess);
	BuildRequiredResourceBarriers(passRecord, declaration, compiledResource);

	lastAccess.Pass = passIndex;
	lastAccess.Queue = passRecord.queue;
}

void FrameGraphCompiler::BuildQueueOwnershipBarriers(
    FrameGraphPassIndex passIndex,
    FrameGraphPassNode& passRecord,
    const PassResourceDeclaration& declaration,
    FrameGraphResourceNode& compiledResource,
    const LastResourceAccess& lastAccess) noexcept
{
	const bool crossesQueue =
	    lastAccess.Pass != INVALID_FRAME_GRAPH_PASS_INDEX && lastAccess.Pass != passIndex && lastAccess.Queue != passRecord.queue;
	if (crossesQueue)
	{
		AddSynchronizationDependency(passRecord, lastAccess.Pass);
		if (compiledResource.currentState != ResourceState::Common
		    && compiledResource.currentState != ResourceState::RayTracingAccelerationStructure)
		{
			m_plan.passes[lastAccess.Pass].compiledReleaseBarriers.push_back(
			    FrameGraphBarrier{
			        .handle = declaration.handle,
			        .type = FrameGraphBarrier::Type::Transition,
			        .before = compiledResource.currentState,
			        .after = ResourceState::Common,
			        .label = "QueueRelease"});
			compiledResource.currentState = ResourceState::Common;
		}

		return;
	}

	const bool startsOnNonGraphicsQueue = lastAccess.Pass == INVALID_FRAME_GRAPH_PASS_INDEX && passRecord.queue != ERhiQueueType::Graphics;
	if (startsOnNonGraphicsQueue && compiledResource.currentState != ResourceState::Common
	    && compiledResource.currentState != ResourceState::RayTracingAccelerationStructure)
	{
		m_plan.initialBarriers.push_back(
		    FrameGraphBarrier{
		        .handle = declaration.handle,
		        .type = FrameGraphBarrier::Type::Transition,
		        .before = compiledResource.currentState,
		        .after = ResourceState::Common,
		        .label = "InitialQueueRelease"});
		compiledResource.currentState = ResourceState::Common;
	}
}

void FrameGraphCompiler::BuildRequiredResourceBarriers(
    FrameGraphPassNode& passRecord,
    const PassResourceDeclaration& declaration,
    FrameGraphResourceNode& compiledResource) noexcept
{
	const ResourceState requiredState = InferRequiredResourceState(declaration, compiledResource);
	if (FrameGraphCompilerRayTracing::UsesRayTracingState(declaration))
	{
		if (FrameGraphCompilerRayTracing::RequiresTransitionBarrier(compiledResource.currentState, requiredState))
		{
			passRecord.compiledBarriers.push_back(
			    FrameGraphBarrier{
			        .handle = declaration.handle,
			        .type = FrameGraphBarrier::Type::Transition,
			        .before = compiledResource.currentState,
			        .after = requiredState,
			        .label = declaration.label});
			compiledResource.currentState = requiredState;
			m_resourceStateTracker.UpdateCurrentState(declaration.handle, requiredState);
		}

		if (compiledResource.pendingAccelerationStructureBarrier
		    && !HasCompiledBarrier(passRecord, declaration.handle, FrameGraphBarrier::Type::AccelerationStructure))
		{
			passRecord.compiledBarriers.push_back(
			    FrameGraphBarrier{
			        .handle = declaration.handle,
			        .type = FrameGraphBarrier::Type::AccelerationStructure,
			        .before = requiredState,
			        .after = requiredState,
			        .label = declaration.label});
		}

		compiledResource.pendingAccelerationStructureBarrier = declaration.usage == ResourceUsage::AccelerationStructureBuild;
		return;
	}

	if (compiledResource.currentState != requiredState)
	{
		passRecord.compiledBarriers.push_back(
		    FrameGraphBarrier{
		        .handle = declaration.handle,
		        .type = FrameGraphBarrier::Type::Transition,
		        .before = compiledResource.currentState,
		        .after = requiredState,
		        .label = declaration.label});
		compiledResource.currentState = requiredState;
		m_resourceStateTracker.UpdateCurrentState(declaration.handle, requiredState);
		return;
	}

	const bool requiresUnorderedAccessBarrier = requiredState == ResourceState::UnorderedAccess && WritesToUsage(declaration.usage)
	    && !HasCompiledBarrier(passRecord, declaration.handle, FrameGraphBarrier::Type::UnorderedAccess);
	if (requiresUnorderedAccessBarrier)
	{
		passRecord.compiledBarriers.push_back(
		    FrameGraphBarrier{
		        .handle = declaration.handle,
		        .type = FrameGraphBarrier::Type::UnorderedAccess,
		        .before = requiredState,
		        .after = requiredState,
		        .label = declaration.label});
	}
}

void FrameGraphCompiler::BuildFinalResourceBarriers(const LastResourceAccessTable& lastResourceAccesses) noexcept
{
	for (FrameGraphResourceNode& compiledResource : m_plan.resources)
	{
		if (!ShouldRestoreFinalState(compiledResource))
		{
			continue;
		}

		const FrameGraphResourceMetadata& entry = m_resourceRegistry.GetMetadata(compiledResource.handle);
		const LastResourceAccess& lastAccess = lastResourceAccesses[compiledResource.index];
		if (lastAccess.Pass != INVALID_FRAME_GRAPH_PASS_INDEX && lastAccess.Queue != ERhiQueueType::Graphics
		    && compiledResource.currentState != ResourceState::Common
		    && compiledResource.currentState != ResourceState::RayTracingAccelerationStructure)
		{
			m_plan.passes[lastAccess.Pass].compiledReleaseBarriers.push_back(
			    FrameGraphBarrier{
			        .handle = entry.handle,
			        .type = FrameGraphBarrier::Type::Transition,
			        .before = compiledResource.currentState,
			        .after = ResourceState::Common,
			        .label = "FinalQueueRelease"});
			compiledResource.currentState = ResourceState::Common;
		}

		if (compiledResource.currentState == compiledResource.finalState)
		{
			continue;
		}

		m_plan.finalBarriers.push_back(
		    FrameGraphBarrier{
		        .handle = entry.handle,
		        .type = FrameGraphBarrier::Type::Transition,
		        .before = compiledResource.currentState,
		        .after = compiledResource.finalState,
		        .label = "FinalState"});
		compiledResource.currentState = compiledResource.finalState;
		m_resourceStateTracker.UpdateCurrentState(compiledResource.handle, entry.finalState);
	}
}

bool FrameGraphCompiler::HasCompiledBarrier(
    const FrameGraphPassNode& passRecord,
    FrameGraphResourceHandle handle,
    FrameGraphBarrier::Type type) noexcept
{
	return std::find_if(
	           passRecord.compiledBarriers.begin(),
	           passRecord.compiledBarriers.end(),
	           [handle, type](const FrameGraphBarrier& barrier) { return barrier.handle == handle && barrier.type == type; })
	    != passRecord.compiledBarriers.end();
}
