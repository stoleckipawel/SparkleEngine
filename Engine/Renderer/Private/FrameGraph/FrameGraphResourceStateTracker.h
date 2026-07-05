#pragma once

#include "FrameGraph/FrameGraphResourceRegistry.h"

#include <vector>

class FrameGraphResourceStateTracker final
{
  public:
	FrameGraphResourceStateTracker() = default;
	~FrameGraphResourceStateTracker() = default;

	FrameGraphResourceStateTracker(const FrameGraphResourceStateTracker&) = delete;
	FrameGraphResourceStateTracker& operator=(const FrameGraphResourceStateTracker&) = delete;
	FrameGraphResourceStateTracker(FrameGraphResourceStateTracker&&) = delete;
	FrameGraphResourceStateTracker& operator=(FrameGraphResourceStateTracker&&) = delete;

	void Clear() noexcept;
	void RegisterResource(FrameGraphResourceHandle handle, ResourceState initialState) noexcept;
	void ResetCurrentStates(const FrameGraphResourceRegistry& registry) noexcept;
	void UpdateCurrentState(FrameGraphResourceHandle handle, ResourceState currentState) noexcept;

	ResourceState GetRuntimeState(FrameGraphResourceHandle handle) const noexcept;

  private:
	void EnsureStorage(FrameGraphResourceHandle handle) noexcept;

	std::vector<ResourceState> m_runtimeStates;
	std::vector<bool> m_trackedResources;
};
