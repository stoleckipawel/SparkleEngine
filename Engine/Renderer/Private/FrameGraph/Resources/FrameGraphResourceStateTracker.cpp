#include "PCH.h"
#include "FrameGraph/FrameGraphResourceStateTracker.h"

#include <cassert>

void FrameGraphResourceStateTracker::Clear() noexcept
{
	m_runtimeStates.clear();
	m_trackedResources.clear();
}

void FrameGraphResourceStateTracker::RegisterResource(FrameGraphResourceHandle handle, ResourceState initialState) noexcept
{
	EnsureStorage(handle);
	if (!m_trackedResources[handle.index])
	{
		m_runtimeStates[handle.index].currentState = initialState;
		m_trackedResources[handle.index] = true;
	}
}

void FrameGraphResourceStateTracker::ResetCurrentStates(const FrameGraphResourceRegistry& registry) noexcept
{
	for (const FrameGraphResourceHandle handle : registry.GetRegisteredHandles())
	{
		UpdateCurrentState(handle, registry.GetMetadata(handle).initialState);
	}
}

void FrameGraphResourceStateTracker::UpdateCurrentState(FrameGraphResourceHandle handle, ResourceState currentState) noexcept
{
	GetRuntimeState(handle).currentState = currentState;
}

FrameGraphResourceRuntimeState& FrameGraphResourceStateTracker::GetRuntimeState(FrameGraphResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	assert(handle.index < m_runtimeStates.size() && "FrameGraph resource state is not tracked.");
	assert(handle.index < m_trackedResources.size() && m_trackedResources[handle.index] && "FrameGraph resource state is not tracked.");
	return m_runtimeStates[handle.index];
}

const FrameGraphResourceRuntimeState& FrameGraphResourceStateTracker::GetRuntimeState(FrameGraphResourceHandle handle) const noexcept
{
	assert(handle.IsValid());
	assert(handle.index < m_runtimeStates.size() && "FrameGraph resource state is not tracked.");
	assert(handle.index < m_trackedResources.size() && m_trackedResources[handle.index] && "FrameGraph resource state is not tracked.");
	return m_runtimeStates[handle.index];
}

void FrameGraphResourceStateTracker::EnsureStorage(FrameGraphResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	const std::size_t requiredSize = static_cast<std::size_t>(handle.index) + 1;
	if (m_runtimeStates.size() < requiredSize)
	{
		m_runtimeStates.resize(requiredSize);
		m_trackedResources.resize(requiredSize, false);
	}
}
