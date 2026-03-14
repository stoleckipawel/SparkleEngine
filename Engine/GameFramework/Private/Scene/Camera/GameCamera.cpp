#include "PCH.h"
#include "GameCamera.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

GameCamera::GameCamera() noexcept
{
	UpdateCachedDirection();
}

void GameCamera::Move(const XMFLOAT3& direction, float distance) noexcept
{
	m_transform.TranslateScaled(direction, distance);
	MarkDirty();
}

void GameCamera::MoveForward(float distance) noexcept
{
	Move(GetDirection(), distance);
}

void GameCamera::MoveRight(float distance) noexcept
{
	Move(GetRight(), distance);
}

void GameCamera::MoveUp(float distance) noexcept
{
	const XMFLOAT3 worldUp = {0.0f, 1.0f, 0.0f};
	Move(worldUp, distance);
}

void GameCamera::Rotate(float yawDelta, float pitchDelta) noexcept
{
	constexpr float maxPitch = XM_PIDIV2 - 0.001f;
	m_transform.RotateYawPitch(yawDelta, pitchDelta, -maxPitch, maxPitch);
	m_directionDirty = true;
	MarkDirty();
}

void GameCamera::SetPosition(const XMFLOAT3& position) noexcept
{
	m_transform.SetTranslation(position);
	MarkDirty();
}

void GameCamera::SetAspectRatio(float aspectRatio) noexcept
{
	m_aspectRatio = aspectRatio;
	MarkDirty();
}

void GameCamera::UpdateCachedDirection() const noexcept
{
	const XMFLOAT3 rotationEuler = m_transform.GetRotationEuler();
	const float pitch = rotationEuler.x;
	const float yaw = rotationEuler.y;
	const float cosPitch = std::cos(pitch);
	m_cachedDirection = XMFLOAT3{std::sin(yaw) * cosPitch, std::sin(pitch), std::cos(yaw) * cosPitch};
	m_directionDirty = false;
}

const XMFLOAT3& GameCamera::GetDirection() const noexcept
{
	if (m_directionDirty)
	{
		UpdateCachedDirection();
	}
	return m_cachedDirection;
}

XMFLOAT3 GameCamera::GetRight() const noexcept
{
	const float yaw = m_transform.GetRotationEuler().y;
	return XMFLOAT3{std::cos(yaw), 0.0f, -std::sin(yaw)};
}

void GameCamera::SetYawPitch(float yawRadians, float pitchRadians) noexcept
{
	constexpr float maxPitch = XM_PIDIV2 - 0.01f;
	m_transform.SetYawPitch(yawRadians, pitchRadians, -maxPitch, maxPitch);
	m_directionDirty = true;
	MarkDirty();
}
