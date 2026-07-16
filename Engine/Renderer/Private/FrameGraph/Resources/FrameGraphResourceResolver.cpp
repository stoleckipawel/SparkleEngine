#include "PCH.h"
#include "FrameGraph/FrameGraphResourceResolver.h"

#include <cassert>

void FrameGraphResourceResolver::Clear() noexcept
{
	m_resolvedAccessEntries.clear();
}

void FrameGraphResourceResolver::ClearResolvedAccess(FrameGraphResourceHandle handle) noexcept
{
	SetResolvedAccess(handle, FrameGraphResourceAccess{});
}

void FrameGraphResourceResolver::RegisterResource(FrameGraphResourceHandle handle, RhiResourceHandle resource) noexcept
{
	FrameGraphResourceAccess access{};
	access.resource = resource;
	SetResolvedAccess(handle, access);
}

void FrameGraphResourceResolver::SetResolvedAccess(FrameGraphResourceHandle handle, const FrameGraphResourceAccess& access) noexcept
{
	EnsureStorage(handle);
	m_resolvedAccessEntries[handle.index] = access;
}

FrameGraphResourceAccess& FrameGraphResourceResolver::GetResolvedAccess(FrameGraphResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	assert(handle.index < m_resolvedAccessEntries.size() && "FrameGraph resource access is not resolved.");
	return m_resolvedAccessEntries[handle.index];
}

const FrameGraphResourceAccess& FrameGraphResourceResolver::GetResolvedAccess(FrameGraphResourceHandle handle) const noexcept
{
	assert(handle.IsValid());
	assert(handle.index < m_resolvedAccessEntries.size() && "FrameGraph resource access is not resolved.");
	return m_resolvedAccessEntries[handle.index];
}

void FrameGraphResourceResolver::EnsureStorage(FrameGraphResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	const std::size_t requiredSize = static_cast<std::size_t>(handle.index) + 1;
	if (m_resolvedAccessEntries.size() < requiredSize)
	{
		m_resolvedAccessEntries.resize(requiredSize);
	}
}
