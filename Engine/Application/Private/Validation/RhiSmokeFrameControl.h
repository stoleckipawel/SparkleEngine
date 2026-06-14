#pragma once

#include "Validation/RhiSmokeCameraMotion.h"
#include "Validation/RhiSmokeLevelSwitching.h"

#include <cstdint>
class RuntimeApplication;

struct RhiSmokeFrameControlConfig final
{
	bool Enabled = false;
	std::uint32_t FrameLimit = 120;
	std::uint32_t RestoreFrame = 10;
	std::uint32_t MaximizeFrame = 20;
	std::uint32_t ShaderReloadFrame = 0;
	bool LevelSwitching = true;
	std::uint32_t LevelSwitchIntervalFrames = 15;
	RhiSmokeCameraMotionConfig CameraMotion;
};

struct RhiSmokeFrameControlState final
{
	std::uint32_t CompletedRenderFrames = 0;
	bool Failed = false;
	RhiSmokeLevelSwitchingState LevelSwitching;
	RhiSmokeCameraMotionState CameraMotion;
};

namespace RhiSmokeFrameControl
{
	void InitializeLevelSwitching(
	    const RhiSmokeFrameControlConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeFrameControlState& state) noexcept;
	void Advance(
	    const RhiSmokeFrameControlConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeFrameControlState& state,
	    const char* validationContext) noexcept;
}
