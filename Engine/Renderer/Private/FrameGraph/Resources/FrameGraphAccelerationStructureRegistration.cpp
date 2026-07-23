#include "PCH.h"

#include "FrameGraph/FrameGraph.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <format>

static const auto g_frameGraphAccelerationStructureLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

class FrameGraphAccelerationStructureRegistrationOperations final
{
  public:
	static FrameGraphAccelerationStructureDesc ResolveAccelerationStructureDesc(
	    const FrameGraphAccelerationStructureDesc& desc,
	    std::string_view fallbackName)
	{
		FrameGraphAccelerationStructureDesc resolvedDesc = desc;
		if (resolvedDesc.name.empty())
		{
			resolvedDesc.name = std::string(fallbackName);
		}

		return resolvedDesc;
	}

	static std::string FormatHandle(FrameGraphResourceHandle handle)
	{
		return handle.IsValid() ? std::format("{}", handle.index) : "invalid";
	}

	static void FailInvalidAccelerationStructureBinding(
	    std::string_view operation,
	    std::string_view resourceName,
	    FrameGraphResourceHandle handle,
	    ResourceState state,
	    bool hasResource,
	    RhiGpuVirtualAddress gpuAddress) noexcept
	{
		Diagnostics::Fail(
		    g_frameGraphAccelerationStructureLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph acceleration-structure validation failed: operation='{}' resource='{}' handle={} state={} hasResource={} gpuAddress={} remediation='bind acceleration structures through a valid frame-graph handle with a native backing resource and GPU virtual address'",
		        operation,
		        resourceName.empty() ? "<unnamed>" : resourceName,
		        FormatHandle(handle),
		        ResourceStateToString(state),
		        hasResource,
		        gpuAddress));
	}
};

FrameGraphAccelerationStructureHandle FrameGraph::ReservePersistentAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    ResourceState initialState) noexcept
{
	const FrameGraphAccelerationStructureDesc resolvedDesc = FrameGraphAccelerationStructureRegistrationOperations::ResolveAccelerationStructureDesc(desc, "PersistentAccelerationStructure");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentAccelerationStructure(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphAccelerationStructureHandle{handle};
}

void FrameGraph::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    RhiResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState currentState) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	if (!resource || gpuAddress == 0)
	{
		FrameGraphAccelerationStructureRegistrationOperations::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    static_cast<bool>(resource),
		    gpuAddress);
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		FrameGraphAccelerationStructureRegistrationOperations::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    {},
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource),
		    gpuAddress);
	}

	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	if (metadata.kind != FrameGraphResourceKind::AccelerationStructure
	    || metadata.ownership != FrameGraphResourceOwnership::ExternalPersistent)
	{
		FrameGraphAccelerationStructureRegistrationOperations::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    metadata.debugName,
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource),
		    gpuAddress);
	}

	FrameGraphResourceAccess access{};
	access.resource = resource;
	access.accelerationStructureGpuAddress = gpuAddress;
	m_resourceResolver.SetResolvedAccess(resourceHandle, access);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, currentState);
}

void FrameGraph::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    RhiOwnedResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState currentState) noexcept
{
	if (m_renderHardwareInterface == nullptr || !resource)
	{
		FrameGraphAccelerationStructureRegistrationOperations::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    false,
		    gpuAddress);
	}

	BindPersistentAccelerationStructure(
	    handle,
	    m_renderHardwareInterface->GetResourceService().GetResourceHandle(resource),
	    gpuAddress,
	    currentState);
}

void FrameGraph::ClearPersistentAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return;
	}

	m_resourceResolver.ClearResolvedAccess(resourceHandle);
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, metadata.initialState);
}

RhiGpuVirtualAddress FrameGraph::ResolveAccelerationStructureGpuAddress(FrameGraphResourceHandle handle) const noexcept
{
	assert(handle.IsValid());
	return m_resourceResolver.GetResolvedAccess(handle).accelerationStructureGpuAddress;
}
