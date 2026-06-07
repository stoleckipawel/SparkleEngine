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

RhiGpuVirtualAddress FrameGraph::ResolveAccelerationStructureGpuAddress(FrameGraphResourceHandle handle) const noexcept
{
	assert(handle.IsValid());
	return m_resourceResolver.GetResolvedAccess(handle).accelerationStructureGpuAddress;
}
