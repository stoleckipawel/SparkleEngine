#pragma once

#include <cstdint>

class RuntimeApplication;
struct RendererSmokeDiagnosticsSnapshot;

struct RhiSmokeCameraMotionConfig final
{
	bool Enabled = false;
	bool RequiresRayTracing = false;
	std::uint32_t StartFrame = 30;
	std::uint32_t EndFrame = 90;
	std::uint32_t YawDegrees = 35;
	std::uint32_t PitchDegrees = 0;
};

struct RhiSmokeCameraMotionState final
{
	bool Started = false;
	bool Completed = false;
	bool MissingScene = false;
	std::uint32_t AppliedFrames = 0;
	float InitialYawRadians = 0.0f;
	float InitialPitchRadians = 0.0f;
};

namespace RhiSmokeCameraMotion
{
	RhiSmokeCameraMotionConfig LoadConfig() noexcept;
	void Advance(
	    const RhiSmokeCameraMotionConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeCameraMotionState& state) noexcept;
	bool Validate(
	    const RhiSmokeCameraMotionConfig& config,
	    const RhiSmokeCameraMotionState& state,
	    const RendererSmokeDiagnosticsSnapshot& snapshot,
	    const char* label) noexcept;
}
