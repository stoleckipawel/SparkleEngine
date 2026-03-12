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
	XMVECTOR pos = XMLoadFloat3(&m_position);
	XMVECTOR dir = XMLoadFloat3(&direction);
	pos = XMVectorAdd(pos, XMVectorScale(dir, distance));
	XMStoreFloat3(&m_position, pos);
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
	m_yaw += yawDelta;
	m_pitch += pitchDelta;

	constexpr float maxPitch = XM_PIDIV2 - 0.001f;
	m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);

	m_yaw = std::fmod(m_yaw, XM_2PI);
	if (m_yaw < 0.0f)
	{
		m_yaw += XM_2PI;
	}

	m_directionDirty = true;
	MarkDirty();
}

void GameCamera::SetPosition(const XMFLOAT3& position) noexcept
{
	m_position = position;
	MarkDirty();
}

void GameCamera::SetAspectRatio(float aspectRatio) noexcept
{
	m_aspectRatio = aspectRatio;
	MarkDirty();
}

void GameCamera::UpdateCachedDirection() const noexcept
{
	const float cosPitch = std::cos(m_pitch);
	m_cachedDirection = XMFLOAT3{std::sin(m_yaw) * cosPitch, std::sin(m_pitch), std::cos(m_yaw) * cosPitch};
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
	return XMFLOAT3{std::cos(m_yaw), 0.0f, -std::sin(m_yaw)};
}

void GameCamera::SetYawPitch(float yawRadians, float pitchRadians) noexcept
{
	m_yaw = yawRadians;
	m_pitch = pitchRadians;

	constexpr float maxPitch = XM_PIDIV2 - 0.01f;
	m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);

	m_directionDirty = true;
	MarkDirty();
}
