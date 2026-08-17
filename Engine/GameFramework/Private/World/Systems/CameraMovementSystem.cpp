#include "PCH.h"

#include "World/Systems/CameraMovementSystem.h"

#include "Core/Public/Math/WorldCoordinateSystem.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"

#include <algorithm>
#include <cmath>

namespace ECS
{
	bool CameraMovementSystem::Apply(
	    EntityId entity,
	    EntityId activeCamera,
	    const CameraInputIntent& intent,
	    float deltaSeconds,
	    Camera& camera,
	    CameraMovement& movement,
	    LocalTransform& transform) noexcept
	{
		if (entity != activeCamera || !camera.Active)
			return false;
		bool changed = false;
		if (intent.HasAspectRatio && intent.AspectRatio > 0.0f && camera.AspectRatio != intent.AspectRatio)
		{
			camera.AspectRatio = intent.AspectRatio;
			changed = true;
		}
		if (intent.SpeedStepCount != 0.0f)
		{
			movement.MoveSpeedMetersPerSecond = std::clamp(
			    movement.MoveSpeedMetersPerSecond + intent.SpeedStepCount * movement.SpeedStepMetersPerSecond,
			    movement.MinimumMoveSpeedMetersPerSecond,
			    movement.MaximumMoveSpeedMetersPerSecond);
			changed = true;
		}
		if (intent.LookDeltaX != 0.0f || intent.LookDeltaY != 0.0f)
		{
			DirectX::XMFLOAT3 euler{};
			const float sinPitch = 2.0f * (transform.Rotation.w * transform.Rotation.x - transform.Rotation.z * transform.Rotation.y);
			euler.x = std::asin(std::clamp(sinPitch, -1.0f, 1.0f));
			euler.y = std::atan2(
			    2.0f * (transform.Rotation.w * transform.Rotation.y + transform.Rotation.x * transform.Rotation.z),
			    1.0f - 2.0f * (transform.Rotation.x * transform.Rotation.x + transform.Rotation.y * transform.Rotation.y));
			const float ySign = movement.InvertY ? -1.0f : 1.0f;
			constexpr float MaxPitch = DirectX::XM_PIDIV2 - 0.01f;
			const float pitch = std::clamp(euler.x + ySign * intent.LookDeltaY * movement.MouseSensitivity, -MaxPitch, MaxPitch);
			const float yaw = euler.y + intent.LookDeltaX * movement.MouseSensitivity;
			DirectX::XMStoreFloat4(&transform.Rotation, DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, 0.0f));
			changed = true;
		}
		const float clampedDelta = (std::max) (0.0f, deltaSeconds);
		if (clampedDelta > 0.0f && (intent.ForwardAxis != 0.0f || intent.RightAxis != 0.0f || intent.UpAxis != 0.0f))
		{
			const float speed = movement.MoveSpeedMetersPerSecond * (intent.Sprint ? movement.SprintMultiplier : 1.0f);
			const float distance = speed * clampedDelta;
			const DirectX::XMVECTOR orientation = DirectX::XMLoadFloat4(&transform.Rotation);
			const DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
			    DirectX::XMVectorSet(WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ, 0.0f),
			    orientation);
			const float yaw = std::atan2(
			    2.0f * (transform.Rotation.w * transform.Rotation.y + transform.Rotation.x * transform.Rotation.z),
			    1.0f - 2.0f * (transform.Rotation.x * transform.Rotation.x + transform.Rotation.y * transform.Rotation.y));
			const DirectX::XMVECTOR right = DirectX::XMVectorSet(std::cos(yaw), 0.0f, -std::sin(yaw), 0.0f);
			DirectX::XMVECTOR translation = DirectX::XMLoadFloat3(&transform.Translation);
			translation = DirectX::XMVectorMultiplyAdd(forward, DirectX::XMVectorReplicate(distance * intent.ForwardAxis), translation);
			translation = DirectX::XMVectorMultiplyAdd(right, DirectX::XMVectorReplicate(distance * intent.RightAxis), translation);
			translation = DirectX::XMVectorAdd(
			    translation,
			    DirectX::XMVectorSet(
			        WorldCoordinates::kUpX * distance * intent.UpAxis,
			        WorldCoordinates::kUpY * distance * intent.UpAxis,
			        WorldCoordinates::kUpZ * distance * intent.UpAxis,
			        0.0f));
			DirectX::XMStoreFloat3(&transform.Translation, translation);
			changed = true;
		}
		return changed;
	}
}
