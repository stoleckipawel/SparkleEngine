#include "PCH.h"

#include "Validation/RhiSmokeCameraMotion.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "GameFramework/Public/Scene/Camera/CameraComponent.h"
#include "GameFramework/Public/Scene/Camera/SceneCamera.h"
#include "GameFramework/Public/Scene/GameScene.h"
#include "Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RuntimeApplication.h"

#include <DirectXMath.h>
#include <algorithm>

namespace
{
	float DegreesToRadians(std::uint32_t degrees) noexcept
	{
		return DirectX::XMConvertToRadians(static_cast<float>(degrees));
	}

	std::uint32_t NormalizeEndFrame(std::uint32_t startFrame, std::uint32_t endFrame) noexcept
	{
		return std::max(startFrame + 1u, endFrame);
	}
}

RhiSmokeCameraMotionConfig RhiSmokeCameraMotion::LoadConfig() noexcept
{
	RhiSmokeCameraMotionConfig config{};
	config.Enabled = Environment::GetFlag("SPARKLE_SMOKE_CAMERA_MOTION");
	if (!config.Enabled)
	{
		return config;
	}

	config.RequiresRayTracing = Environment::GetFlag("SPARKLE_SMOKE_CAMERA_MOTION_REQUIRES_RT");
	config.StartFrame = Environment::GetUInt32("SPARKLE_SMOKE_CAMERA_MOTION_START_FRAME", config.StartFrame);
	config.EndFrame = NormalizeEndFrame(
	    config.StartFrame,
	    Environment::GetUInt32("SPARKLE_SMOKE_CAMERA_MOTION_END_FRAME", config.EndFrame));
	config.YawDegrees = Environment::GetUInt32("SPARKLE_SMOKE_CAMERA_MOTION_YAW_DEGREES", config.YawDegrees);
	config.PitchDegrees = Environment::GetUInt32("SPARKLE_SMOKE_CAMERA_MOTION_PITCH_DEGREES", config.PitchDegrees);
	return config;
}

void RhiSmokeCameraMotion::Advance(
    const RhiSmokeCameraMotionConfig& config,
    RuntimeApplication& app,
    std::uint32_t completedRenderFrames,
    RhiSmokeCameraMotionState& state) noexcept
{
	if (!config.Enabled || completedRenderFrames < config.StartFrame || completedRenderFrames > config.EndFrame)
	{
		return;
	}

	GameScene* scene = app.GetGameScene();
	if (scene == nullptr)
	{
		state.MissingScene = true;
		return;
	}

	CameraComponent& camera = scene->GetCameras().GetActiveCamera().GetCameraComponent();
	if (!state.Started)
	{
		const DirectX::XMFLOAT3 rotation = camera.GetTransform().GetRotationEuler();
		state.InitialPitchRadians = rotation.x;
		state.InitialYawRadians = rotation.y;
		state.Started = true;
	}

	const std::uint32_t motionFrameCount = std::max(1u, config.EndFrame - config.StartFrame);
	const std::uint32_t frameOffset = std::min(completedRenderFrames - config.StartFrame, motionFrameCount);
	const float t = static_cast<float>(frameOffset) / static_cast<float>(motionFrameCount);
	const float yaw = state.InitialYawRadians + DegreesToRadians(config.YawDegrees) * t;
	const float pitch = state.InitialPitchRadians + DegreesToRadians(config.PitchDegrees) * t;
	camera.SetYawPitch(yaw, pitch);
	++state.AppliedFrames;

	if (completedRenderFrames >= config.EndFrame)
	{
		state.Completed = true;
	}
}

bool RhiSmokeCameraMotion::Validate(
    const RhiSmokeCameraMotionConfig& config,
    const RhiSmokeCameraMotionState& state,
    const RendererSmokeDiagnosticsSnapshot& snapshot,
    const char* label) noexcept
{
	if (!config.Enabled)
	{
		return true;
	}

	if (state.MissingScene || !state.Started || !state.Completed || state.AppliedFrames == 0)
	{
		return false;
	}

	if (config.RequiresRayTracing && (!snapshot.RayTracing.Capability.Supported || !snapshot.RayTracing.Capability.InlineRayQuerySupported ||
	                                 !snapshot.RayTracing.ClassicTlas.Valid || snapshot.RayTracing.ClassicTlas.InstanceCount == 0))
	{
		return false;
	}

	return snapshot.FrameGraph.UnresolvedBarrierWarnings == 0 && snapshot.FrameGraph.MissingExecutionBindings == 0;
}
