#pragma once

#include "FrameGraph/FrameGraphResourceRegistry.h"

#include <vector>

struct FrameGraphResourceRuntimeState
{
	ResourceState currentState = ResourceState::Common;
};

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

	FrameGraphResourceRuntimeState& GetRuntimeState(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceRuntimeState& GetRuntimeState(FrameGraphResourceHandle handle) const noexcept;

  private:
	void EnsureStorage(FrameGraphResourceHandle handle) noexcept;

	std::vector<FrameGraphResourceRuntimeState> m_runtimeStates;
	std::vector<bool> m_trackedResources;
};
