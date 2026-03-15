#include "PCH.h"
#include "FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace
{
	std::string BuildResourceDisplayLabel(ResourceHandle handle, const std::string& debugName)
	{
		const std::string& name = debugName.empty() ? std::string{"Resource"} : debugName;
		return std::string{"#"} + std::to_string(handle.index) + ":" + name;
	}

	std::string BuildResourceEventScopeLabel(ResourceHandle handle,
	                                        FrameGraphResourceClass resourceClass,
	                                        const std::string& debugName)
	{
		const std::string& name = debugName.empty() ? std::string{"Resource"} : debugName;
		std::string label{"FG/Resource/"};
		label += resourceClass == FrameGraphResourceClass::Buffer ? "Buffer/" : "Texture/";
		label += std::to_string(handle.index);
		label += "/";
		label += name;
		return label;
	}

	void ValidateResourceVersionGraph(const FrameGraph::CompiledPlan& plan) noexcept
	{
		for (const FrameGraph::CompileResourceEntry& resource : plan.resources)
		{
			assert(resource.handle.IsValid());
			assert(!resource.versions.empty());
			assert(resource.currentVersion < resource.versions.size());

			for (std::size_t versionIndex = 0; versionIndex < resource.versions.size(); ++versionIndex)
			{
				const FrameGraph::ResourceVersion& version = resource.versions[versionIndex];
				assert(version.handle == resource.handle);
				assert(version.version == versionIndex);
				if (version.writerPass != FrameGraph::INVALID_PASS_INDEX)
				{
					assert(version.writerPass < plan.passes.size());
				}

				for (const FrameGraph::PassIndex readerPass : version.readerPasses)
				{
					assert(readerPass < plan.passes.size());
				}
			}
		}
	}

}

FrameGraphCompiler::FrameGraphCompiler(FrameGraph::CompiledPlan& plan, ResourceRegistry& resourceRegistry) noexcept :
	m_plan(plan), m_resourceRegistry(resourceRegistry)
{
}

void FrameGraphCompiler::Compile() noexcept
{
	m_resourceRegistry.ResetCurrentStates();
	BuildCompiledPlanResources();
	m_plan.executionOrder.clear();
	m_plan.executionOrder.reserve(m_plan.passes.size());
	m_plan.finalBarriers.clear();

	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		passRecord.dependsOn.clear();
		passRecord.successors.clear();
		passRecord.inDegree = 0;
		passRecord.alive = true;
		passRecord.compiledBarriers.clear();
	}

	BuildResourceVersionGraph();
	ValidateResourceVersionGraph(m_plan);
	FinalizePassDependencies();
	BuildTransientResourceLifetimes();
	BuildTransientPhysicalBlockAssignments();
	BuildTransientAliasingBarriers();
	ResetCompiledResourceStatesForBarrierPlanning();

	for (const PassIndex passIndex : m_plan.executionOrder)
	{
		CompilePassRecord& passRecord = m_plan.passes[passIndex];
		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (!declaration.handle.IsValid())
			{
				continue;
			}

			CompileResourceEntry& compiledResource = GetCompiledResourceEntry(declaration.handle);
			FrameGraphResourceRuntimeState& runtimeState = m_resourceRegistry.GetRuntimeState(declaration.handle);
			const ResourceState requiredState = InferRequiredResourceState(declaration, compiledResource);
			if (compiledResource.currentState != requiredState)
			{
				passRecord.compiledBarriers.push_back(
				    CompiledBarrier{.handle = declaration.handle, .before = compiledResource.currentState, .after = requiredState});
				compiledResource.currentState = requiredState;
				runtimeState.currentState = requiredState;
			}
		}
	}

	for (CompileResourceEntry& compiledResource : m_plan.resources)
	{
		if (!ShouldRestoreFinalState(compiledResource))
		{
			continue;
		}

		const FrameGraphResourceMetadata& entry = m_resourceRegistry.GetMetadata(compiledResource.handle);
		FrameGraphResourceRuntimeState& runtimeState = m_resourceRegistry.GetRuntimeState(compiledResource.handle);
		if (compiledResource.currentState == compiledResource.finalState)
		{
			continue;
		}

		m_plan.finalBarriers.push_back(
		    CompiledBarrier{.handle = entry.handle, .before = compiledResource.currentState, .after = compiledResource.finalState});
		compiledResource.currentState = compiledResource.finalState;
		runtimeState.currentState = entry.finalState;
	}
}

void FrameGraphCompiler::BuildCompiledPlanResources() noexcept
{
	m_plan.resources.clear();
	m_plan.resources.reserve(m_resourceRegistry.GetRegisteredHandles().size());

	const std::vector<ResourceHandle>& registeredHandles = m_resourceRegistry.GetRegisteredHandles();
	for (std::size_t resourceIndex = 0; resourceIndex < registeredHandles.size(); ++resourceIndex)
	{
		const ResourceHandle handle = registeredHandles[resourceIndex];
		const FrameGraphResourceMetadata& entry = m_resourceRegistry.GetMetadata(handle);
		const FrameGraphResourceRuntimeState& runtimeState = m_resourceRegistry.GetRuntimeState(handle);
		m_plan.resources.push_back(
		    CompileResourceEntry{.index = static_cast<ResourceIndex>(resourceIndex),
		                         .handle = entry.handle,
		                         .resourceClass = entry.resourceClass,
		                         .kind = entry.kind,
		                         .ownership = entry.ownership,
		                         .initialState = entry.initialState,
		                         .finalState = entry.finalState,
		                         .currentState = runtimeState.currentState,
		                         .debugName = entry.debugName,
		                         .displayLabel = BuildResourceDisplayLabel(entry.handle, entry.debugName),
		                         .eventScopeLabel = BuildResourceEventScopeLabel(entry.handle, entry.resourceClass, entry.debugName),
		                         .currentVersion = 0,
		                         .versions = {ResourceVersion{.handle = entry.handle,
		                                                      .version = 0,
		                                                      .writerPass = FrameGraph::INVALID_PASS_INDEX}}});
	}
}

void FrameGraphCompiler::ResetCompiledResourceStatesForBarrierPlanning() noexcept
{
	for (CompileResourceEntry& compiledResource : m_plan.resources)
	{
		compiledResource.currentState = compiledResource.initialState;
		FrameGraphResourceRuntimeState& runtimeState = m_resourceRegistry.GetRuntimeState(compiledResource.handle);
		runtimeState.currentState = compiledResource.initialState;
	}
}

ResourceState FrameGraphCompiler::InferRequiredResourceState(const PassResourceDeclaration& declaration,
                                                             const CompileResourceEntry& resource) const noexcept
{
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
			default:
			assert(false);
			return ResourceState::Common;
		}
	}

	assert(false);
	return ResourceState::Common;
}

bool FrameGraphCompiler::ShouldRestoreFinalState(const CompileResourceEntry& resource) const noexcept
{
	return resource.ownership != FrameGraphResourceOwnership::Transient || resource.kind == FrameGraphResourceKind::DepthStencil;
}

FrameGraph::ResourceVersion& FrameGraphCompiler::GetCurrentResourceVersion(CompileResourceEntry& resource) noexcept
{
	assert(resource.currentVersion < resource.versions.size());
	return resource.versions[resource.currentVersion];
}

const FrameGraph::ResourceVersion& FrameGraphCompiler::GetCurrentResourceVersion(const CompileResourceEntry& resource) const noexcept
{
	assert(resource.currentVersion < resource.versions.size());
	return resource.versions[resource.currentVersion];
}

FrameGraph::CompileResourceEntry& FrameGraphCompiler::GetCompiledResourceEntry(ResourceHandle handle) noexcept
{
	const auto it = std::find_if(
	    m_plan.resources.begin(),
	    m_plan.resources.end(),
	    [handle](const CompileResourceEntry& resource)
	    {
		    return resource.handle == handle;
	    });
	assert(it != m_plan.resources.end());
	return *it;
}

const FrameGraph::CompileResourceEntry& FrameGraphCompiler::GetCompiledResourceEntry(ResourceHandle handle) const noexcept
{
	const auto it = std::find_if(
	    m_plan.resources.begin(),
	    m_plan.resources.end(),
	    [handle](const CompileResourceEntry& resource)
	    {
		    return resource.handle == handle;
	    });
	assert(it != m_plan.resources.end());
	return *it;
}

FrameGraph::CompiledTransientResourcePlan* FrameGraphCompiler::FindTransientResourcePlan(ResourceHandle handle) noexcept
{
	const auto it = std::find_if(
	    m_plan.transientResources.begin(),
	    m_plan.transientResources.end(),
	    [handle](const FrameGraph::CompiledTransientResourcePlan& transientPlan)
	    {
		    return transientPlan.handle == handle;
	    });

	return it != m_plan.transientResources.end() ? &(*it) : nullptr;
}

const FrameGraph::CompiledTransientResourcePlan* FrameGraphCompiler::FindTransientResourcePlan(ResourceHandle handle) const noexcept
{
	const auto it = std::find_if(
	    m_plan.transientResources.begin(),
	    m_plan.transientResources.end(),
	    [handle](const FrameGraph::CompiledTransientResourcePlan& transientPlan)
	    {
		    return transientPlan.handle == handle;
	    });

	return it != m_plan.transientResources.end() ? &(*it) : nullptr;
}
