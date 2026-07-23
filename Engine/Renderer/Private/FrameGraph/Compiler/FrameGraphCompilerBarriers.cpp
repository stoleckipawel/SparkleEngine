#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphCompiler.h"

#include "FrameGraph/Compiler/FrameGraphCompilerRayTracing.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"

#include <algorithm>

class FrameGraphCompilerBarriersImplementation final
{
  public:
	struct LastResourceAccess final
	{
		FrameGraphPassIndex Pass = INVALID_FRAME_GRAPH_PASS_INDEX;
		ERhiQueueType Queue = ERhiQueueType::Graphics;
	};

	static bool HasCompiledBarrier(
	    const FrameGraphPassNode& passRecord,
	    FrameGraphResourceHandle handle,
	    FrameGraphBarrier::Type type) noexcept
	{
		return std::find_if(
		           passRecord.compiledBarriers.begin(),
		           passRecord.compiledBarriers.end(),
		           [handle, type](const FrameGraphBarrier& barrier)
		           {
			           return barrier.handle == handle && barrier.type == type;
		           }) != passRecord.compiledBarriers.end();
	}
};

void FrameGraphCompiler::BuildResourceBarriers() noexcept
{
	ResetCompiledResourceStatesForBarrierPlanning();
	std::vector<FrameGraphCompilerBarriersImplementation::LastResourceAccess> lastResourceAccesses(m_plan.resources.size());
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.synchronizationDependencies = passRecord.dependsOn;
	}

	for (const FrameGraphPassIndex passIndex : m_plan.executionOrder)
	{
		FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (!declaration.handle.IsValid())
			{
				continue;
			}

			FrameGraphResourceNode& compiledResource = GetCompiledResourceEntry(declaration.handle);
			FrameGraphCompilerBarriersImplementation::LastResourceAccess& lastAccess = lastResourceAccesses[compiledResource.index];
			const bool crossesQueue = lastAccess.Pass != INVALID_FRAME_GRAPH_PASS_INDEX &&
			                          lastAccess.Pass != passIndex && lastAccess.Queue != passRecord.queue;
			if (crossesQueue)
			{
				AddSynchronizationDependency(passRecord, lastAccess.Pass);
				if (compiledResource.currentState != ResourceState::Common &&
				    compiledResource.currentState != ResourceState::RayTracingAccelerationStructure)
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
			}
			else if (
			    lastAccess.Pass == INVALID_FRAME_GRAPH_PASS_INDEX && passRecord.queue != ERhiQueueType::Graphics &&
			    compiledResource.currentState != ResourceState::Common &&
			    compiledResource.currentState != ResourceState::RayTracingAccelerationStructure)
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

				if (compiledResource.pendingAccelerationStructureBarrier &&
				    !FrameGraphCompilerBarriersImplementation::HasCompiledBarrier(passRecord, declaration.handle, FrameGraphBarrier::Type::AccelerationStructure))
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
			}
			else if (compiledResource.currentState != requiredState)
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
			else if (
			    requiredState == ResourceState::UnorderedAccess && WritesToUsage(declaration.usage) &&
			    !FrameGraphCompilerBarriersImplementation::HasCompiledBarrier(passRecord, declaration.handle, FrameGraphBarrier::Type::UnorderedAccess))
			{
				passRecord.compiledBarriers.push_back(
				    FrameGraphBarrier{
				        .handle = declaration.handle,
				        .type = FrameGraphBarrier::Type::UnorderedAccess,
				        .before = requiredState,
				        .after = requiredState,
				        .label = declaration.label});
			}

			lastAccess.Pass = passIndex;
			lastAccess.Queue = passRecord.queue;
		}
	}

	for (FrameGraphResourceNode& compiledResource : m_plan.resources)
	{
		if (!ShouldRestoreFinalState(compiledResource))
		{
			continue;
		}

		const FrameGraphResourceMetadata& entry = m_resourceRegistry.GetMetadata(compiledResource.handle);
		const FrameGraphCompilerBarriersImplementation::LastResourceAccess& lastAccess = lastResourceAccesses[compiledResource.index];
		if (lastAccess.Pass != INVALID_FRAME_GRAPH_PASS_INDEX && lastAccess.Queue != ERhiQueueType::Graphics &&
		    compiledResource.currentState != ResourceState::Common &&
		    compiledResource.currentState != ResourceState::RayTracingAccelerationStructure)
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
