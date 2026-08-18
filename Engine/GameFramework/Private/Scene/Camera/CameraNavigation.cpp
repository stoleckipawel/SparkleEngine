#include "PCH.h"

#include "GameFramework/Public/Scene/Camera/CameraNavigation.h"

#include "Core/Public/Math/WorldCoordinateSystem.h"

#include <algorithm>
#include <cmath>

bool CameraNavigation::Apply(
    const CameraInputIntent& intent,
    const CameraNavigationSettings& settings,
    float deltaSeconds,
    CameraNavigationState& state) noexcept
{
	bool changed = false;
	if (intent.LookDeltaX != 0.0f || intent.LookDeltaY != 0.0f)
	{
		const float rotationSpeedRadiansPerPixel = DirectX::XMConvertToRadians(settings.RotationSpeedDegreesPerPixel);
		const float ySign = settings.InvertY ? -1.0f : 1.0f;
		constexpr float maximumPitch = DirectX::XM_PIDIV2 - 0.01f;
		state.PitchRadians =
		    std::clamp(state.PitchRadians + ySign * intent.LookDeltaY * rotationSpeedRadiansPerPixel, -maximumPitch, maximumPitch);
		state.YawRadians += intent.LookDeltaX * rotationSpeedRadiansPerPixel;
		changed = true;
	}

	const float clampedDeltaSeconds = (std::max) (0.0f, deltaSeconds);
	if (clampedDeltaSeconds <= 0.0f || (intent.ForwardAxis == 0.0f && intent.RightAxis == 0.0f && intent.UpAxis == 0.0f))
	{
		return changed;
	}

	const float speed = settings.MoveSpeedMetersPerSecond * (intent.Sprint ? settings.SprintMultiplier : 1.0f);
	const float distance = speed * clampedDeltaSeconds;
	const DirectX::XMVECTOR orientation = DirectX::XMQuaternionRotationRollPitchYaw(state.PitchRadians, state.YawRadians, 0.0f);
	const DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
	    DirectX::XMVectorSet(WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ, 0.0f),
	    orientation);
	const DirectX::XMVECTOR right = DirectX::XMVectorSet(std::cos(state.YawRadians), 0.0f, -std::sin(state.YawRadians), 0.0f);
	DirectX::XMVECTOR translation = DirectX::XMLoadFloat3(&state.Position);
	translation = DirectX::XMVectorMultiplyAdd(forward, DirectX::XMVectorReplicate(distance * intent.ForwardAxis), translation);
	translation = DirectX::XMVectorMultiplyAdd(right, DirectX::XMVectorReplicate(distance * intent.RightAxis), translation);
	translation = DirectX::XMVectorAdd(
	    translation,
	    DirectX::XMVectorSet(
	        WorldCoordinates::kUpX * distance * intent.UpAxis,
	        WorldCoordinates::kUpY * distance * intent.UpAxis,
	        WorldCoordinates::kUpZ * distance * intent.UpAxis,
	        0.0f));
	DirectX::XMStoreFloat3(&state.Position, translation);
	return true;
}

DirectX::XMFLOAT3 CameraNavigation::BuildDirection(const CameraNavigationState& state) noexcept
{
	const DirectX::XMVECTOR orientation = DirectX::XMQuaternionRotationRollPitchYaw(state.PitchRadians, state.YawRadians, 0.0f);
	const DirectX::XMVECTOR direction = DirectX::XMVector3Rotate(
	    DirectX::XMVectorSet(WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ, 0.0f),
	    orientation);
	DirectX::XMFLOAT3 result;
	DirectX::XMStoreFloat3(&result, direction);
	return result;
}

float CameraNavigation::ApplySpeedSteps(float speedMetersPerSecond, float stepCount, float minimumSpeed, float maximumSpeed) noexcept
{
	const float safeMinimum = (std::max) (minimumSpeed, 0.0001f);
	const float safeMaximum = (std::max) (maximumSpeed, safeMinimum);
	const float safeSpeed = std::clamp(speedMetersPerSecond, safeMinimum, safeMaximum);
	return std::clamp(safeSpeed * std::pow(1.25f, stepCount), safeMinimum, safeMaximum);
}
