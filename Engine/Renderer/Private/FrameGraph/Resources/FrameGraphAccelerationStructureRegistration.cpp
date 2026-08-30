#include "PCH.h"

#include "FrameGraph/FrameGraph.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <format>

static const auto g_frameGraphAccelerationStructureLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

class FrameGraphAccelerationStructureBindingValidator final
{
public:
	static std::string ResolveName(std::string_view name, std::string_view defaultName)
	{
		return std::string(name.empty() ? defaultName : name);
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
	    bool hasResource) noexcept
	{
		Diagnostics::Fatal(
		    g_frameGraphAccelerationStructureLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph acceleration-structure validation failed: operation='{}' resource='{}' handle={} state={} hasResource={} "
		        "remediation='bind acceleration structures through a valid frame-graph handle with a native backing resource'",
		        operation,
		        resourceName.empty() ? "<unnamed>" : resourceName,
		        FormatHandle(handle),
		        ResourceStateToString(state),
		        hasResource));
	}
};

FrameGraphAccelerationStructureHandle FrameGraph::ReservePersistentAccelerationStructure(
    std::string_view name,
    ResourceState initialState) noexcept
{
	const std::string resolvedName = FrameGraphAccelerationStructureBindingValidator::ResolveName(name, "PersistentAccelerationStructure");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentAccelerationStructure(handle, resolvedName, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphAccelerationStructureHandle{handle};
}

void FrameGraph::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    RhiResourceHandle resource,
    ResourceState currentState) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	if (!resource)
	{
		FrameGraphAccelerationStructureBindingValidator::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    static_cast<bool>(resource));
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		FrameGraphAccelerationStructureBindingValidator::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    {},
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource));
	}

	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	if (metadata.kind != FrameGraphResourceKind::AccelerationStructure
	    || metadata.ownership != FrameGraphResourceOwnership::ExternalPersistent)
	{
		FrameGraphAccelerationStructureBindingValidator::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    metadata.debugName,
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource));
	}

	FrameGraphResourceAccess access{};
	access.resource = resource;
	m_resourceResolver.SetResolvedAccess(resourceHandle, access);
	m_resourceRegistry.SetExternalContentsProduced(resourceHandle, true);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, currentState);
}

void FrameGraph::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    RhiOwnedResourceHandle resource,
    ResourceState currentState) noexcept
{
	if (m_renderHardwareInterface == nullptr || !resource)
	{
		FrameGraphAccelerationStructureBindingValidator::FailInvalidAccelerationStructureBinding(
		    "BindPersistentAccelerationStructure",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    false);
	}

	BindPersistentAccelerationStructure(handle, m_renderHardwareInterface->GetResourceService().GetResourceHandle(resource), currentState);
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
	m_resourceRegistry.SetExternalContentsProduced(resourceHandle, false);
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, metadata.initialState);
}
