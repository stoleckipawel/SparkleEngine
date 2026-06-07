#include "PCH.h"

#include "FrameGraph/FrameGraph.h"

static const auto g_frameGraphAccelerationStructureLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace
{
	FrameGraphAccelerationStructureDesc ResolveAccelerationStructureDesc(
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
}

FrameGraphAccelerationStructureHandle FrameGraph::ImportAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    NativeResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState initialState) noexcept
{
	if (!resource || gpuAddress == 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_frameGraphAccelerationStructureLogger,
		    "FrameGraph::ImportAccelerationStructure: acceleration structure is missing a backing resource or GPU address.");
		return FrameGraphAccelerationStructureHandle::Invalid();
	}

	const FrameGraphAccelerationStructureDesc resolvedDesc = ResolveAccelerationStructureDesc(desc, "ImportedAccelerationStructure");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedAccelerationStructure(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	FrameGraphResourceAccess access{};
	access.resource = resource;
	access.accelerationStructureGpuAddress = gpuAddress;
	m_resourceResolver.SetResolvedAccess(handle, access);
	return FrameGraphAccelerationStructureHandle{handle};
}

FrameGraphAccelerationStructureHandle FrameGraph::ImportPersistentAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    NativeResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState initialState) noexcept
{
	if (!resource || gpuAddress == 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_frameGraphAccelerationStructureLogger,
		    "FrameGraph::ImportPersistentAccelerationStructure: persistent acceleration structure is missing a backing resource or GPU address.");
		return FrameGraphAccelerationStructureHandle::Invalid();
	}

	const FrameGraphAccelerationStructureDesc resolvedDesc = ResolveAccelerationStructureDesc(desc, "PersistentAccelerationStructure");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentAccelerationStructure(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	FrameGraphResourceAccess access{};
	access.resource = resource;
	access.accelerationStructureGpuAddress = gpuAddress;
	m_resourceResolver.SetResolvedAccess(handle, access);
	return FrameGraphAccelerationStructureHandle{handle};
}

FrameGraphAccelerationStructureHandle FrameGraph::ReservePersistentAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    ResourceState initialState) noexcept
{
	const FrameGraphAccelerationStructureDesc resolvedDesc = ResolveAccelerationStructureDesc(desc, "PersistentAccelerationStructure");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentAccelerationStructure(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphAccelerationStructureHandle{handle};
}

void FrameGraph::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    NativeResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState currentState) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	if (!resource || gpuAddress == 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_frameGraphAccelerationStructureLogger,
		    "FrameGraph::BindPersistentAccelerationStructure: acceleration structure binding is missing a backing resource or GPU address.");
		ClearPersistentAccelerationStructureBinding(handle);
		return;
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		SPDLOG_LOGGER_WARN(
		    g_frameGraphAccelerationStructureLogger,
		    "FrameGraph::BindPersistentAccelerationStructure: handle {} is not registered.",
		    resourceHandle.index);
		return;
	}

	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	if (metadata.kind != FrameGraphResourceKind::AccelerationStructure
	    || metadata.ownership != FrameGraphResourceOwnership::ExternalPersistent)
	{
		SPDLOG_LOGGER_WARN(
		    g_frameGraphAccelerationStructureLogger,
		    "FrameGraph::BindPersistentAccelerationStructure: handle {} is not a persistent acceleration structure.",
		    resourceHandle.index);
		return;
	}

	FrameGraphResourceAccess access{};
	access.resource = resource;
	access.accelerationStructureGpuAddress = gpuAddress;
	m_resourceResolver.SetResolvedAccess(resourceHandle, access);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, currentState);
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
