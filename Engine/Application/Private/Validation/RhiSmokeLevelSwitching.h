#pragma once

#include <cstdint>
#include <string>
#include <vector>

class RuntimeApplication;

struct RhiSmokeLevelSwitchingConfig final
{
	bool Enabled = true;
	std::uint32_t IntervalFrames = 15;
};

struct RhiSmokeLevelSwitchingState final
{
	bool Initialized = false;
	bool Finished = false;
	std::uint32_t LastSwitchFrame = 0;
	std::uint32_t CompletedSwitches = 0;
	std::vector<std::string> SwitchOrder;
	std::string PendingLevelName;
};

namespace RhiSmokeLevelSwitching
{
	void Initialize(
	    const RhiSmokeLevelSwitchingConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeLevelSwitchingState& state,
	    bool& failed) noexcept;
	void Advance(
	    const RhiSmokeLevelSwitchingConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeLevelSwitchingState& state,
	    bool& failed) noexcept;
}
