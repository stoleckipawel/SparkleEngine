#pragma once

#include <DirectXMath.h>

class GameCamera final
{
  public:
	GameCamera() noexcept;
	~GameCamera() noexcept = default;

	GameCamera(const GameCamera&) = delete;
	GameCamera& operator=(const GameCamera&) = delete;
	GameCamera(GameCamera&&) = delete;
	GameCamera& operator=(GameCamera&&) = delete;

	void Move(const DirectX::XMFLOAT3& direction, float distance) noexcept;

	void MoveForward(float distance) noexcept;

	void MoveRight(float distance) noexcept;

	void MoveUp(float distance) noexcept;

	void Rotate(float yawDelta, float pitchDelta) noexcept;

	DirectX::XMFLOAT3 GetPosition() const noexcept { return m_position; }
	void SetPosition(const DirectX::XMFLOAT3& position) noexcept;

	const DirectX::XMFLOAT3& GetDirection() const noexcept;
	DirectX::XMFLOAT3 GetRight() const noexcept;

	void SetYawPitch(float yawRadians, float pitchRadians) noexcept;
	float GetYaw() const noexcept { return m_yaw; }
	float GetPitch() const noexcept { return m_pitch; }

	bool IsDirty() const noexcept { return m_dirty; }

	void ClearDirty() noexcept { m_dirty = false; }

	float GetFovYDegrees() const noexcept { return m_fovYDegrees; }
	void SetFovYDegrees(float fovDegrees) noexcept { m_fovYDegrees = fovDegrees; }

	float GetNearZ() const noexcept { return m_nearZ; }
	float GetFarZ() const noexcept { return m_farZ; }
	void SetNearFar(float nearZ, float farZ) noexcept
	{
		m_nearZ = nearZ;
		m_farZ = farZ;
	}

	void SetAspectRatio(float aspectRatio) noexcept;
	float GetAspectRatio() const noexcept { return m_aspectRatio; }

  private:
	void UpdateCachedDirection() const noexcept;

	void MarkDirty() noexcept { m_dirty = true; }

	DirectX::XMFLOAT3 m_position = {0.0f, 0.0f, -4.0f};
	float m_pitch = 0.0f;
	float m_yaw = 0.0f;

	mutable DirectX::XMFLOAT3 m_cachedDirection = {0.0f, 0.0f, 1.0f};
	mutable bool m_directionDirty = true;

	float m_fovYDegrees = 60.0f;
	float m_nearZ = 0.1f;
	float m_farZ = 1000.0f;
	float m_aspectRatio = 16.0f / 9.0f;

	bool m_dirty = true;
};
