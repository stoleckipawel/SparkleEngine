#include "PCH.h"
#include "Scene/Camera/CameraComponent.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

CameraComponent::CameraComponent() noexcept
{
	UpdateCachedDirection();
}

void CameraComponent::Move(const XMFLOAT3& direction, float distance) noexcept
{
	m_transform.TranslateScaled(direction, distance);
	MarkDirty();
}

void CameraComponent::MoveForward(float distance) noexcept
{
	Move(GetDirection(), distance);
}

void CameraComponent::MoveRight(float distance) noexcept
{
	Move(GetRight(), distance);
}

void CameraComponent::MoveUp(float distance) noexcept
{
	const XMFLOAT3 worldUp = {0.0f, 1.0f, 0.0f};
	Move(worldUp, distance);
}

void CameraComponent::Rotate(float yawDelta, float pitchDelta) noexcept
{
	constexpr float maxPitch = XM_PIDIV2 - 0.001f;
	m_transform.RotateYawPitch(yawDelta, pitchDelta, -maxPitch, maxPitch);
	m_directionDirty = true;
	MarkDirty();
}

void CameraComponent::SetPosition(const XMFLOAT3& position) noexcept
{
	m_transform.SetTranslation(position);
	MarkDirty();
}

void CameraComponent::SetRotationEuler(const XMFLOAT3& rotationEuler) noexcept
{
	constexpr float maxPitch = XM_PIDIV2 - 0.01f;
	const XMFLOAT3 clampedRotation{std::clamp(rotationEuler.x, -maxPitch, maxPitch), rotationEuler.y, rotationEuler.z};
	m_transform.SetRotationEuler(clampedRotation);
	m_directionDirty = true;
	MarkDirty();
}

void CameraComponent::SetScale(const XMFLOAT3& scale) noexcept
{
	m_transform.SetScale(scale);
	MarkDirty();
}

void CameraComponent::SetAspectRatio(float aspectRatio) noexcept
{
	m_aspectRatio = aspectRatio;
	MarkDirty();
}

void CameraComponent::SetFovYDegrees(float fovDegrees) noexcept
{
	m_fovYDegrees = std::clamp(fovDegrees, 1.0f, 179.0f);
	MarkDirty();
}

void CameraComponent::UpdateCachedDirection() const noexcept
{
	const XMFLOAT3 rotationEuler = m_transform.GetRotationEuler();
	const float pitch = rotationEuler.x;
	const float yaw = rotationEuler.y;
	const float cosPitch = std::cos(pitch);
	m_cachedDirection = XMFLOAT3{std::sin(yaw) * cosPitch, std::sin(pitch), std::cos(yaw) * cosPitch};
	m_directionDirty = false;
}

const XMFLOAT3& CameraComponent::GetDirection() const noexcept
{
	if (m_directionDirty)
	{
		UpdateCachedDirection();
	}
	return m_cachedDirection;
}

XMFLOAT3 CameraComponent::GetRight() const noexcept
{
	const float yaw = m_transform.GetRotationEuler().y;
	return XMFLOAT3{std::cos(yaw), 0.0f, -std::sin(yaw)};
}

void CameraComponent::SetYawPitch(float yawRadians, float pitchRadians) noexcept
{
	constexpr float maxPitch = XM_PIDIV2 - 0.01f;
	m_transform.SetYawPitch(yawRadians, pitchRadians, -maxPitch, maxPitch);
	m_directionDirty = true;
	MarkDirty();
}
