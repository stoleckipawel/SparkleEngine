#pragma once

#include "GameFramework/Public/Scene/Component.h"
#include "GameFramework/Public/Scene/TransformComponent.h"

#include <DirectXMath.h>

class CameraComponent final : public Component
{
  public:
	CameraComponent() noexcept;
	~CameraComponent() override = default;

	CameraComponent(const CameraComponent&) = delete;
	CameraComponent& operator=(const CameraComponent&) = delete;
	CameraComponent(CameraComponent&&) = delete;
	CameraComponent& operator=(CameraComponent&&) = delete;

	void Move(const DirectX::XMFLOAT3& direction, float distance) noexcept;

	void MoveForward(float distance) noexcept;

	void MoveRight(float distance) noexcept;

	void MoveUp(float distance) noexcept;

	void Rotate(float yawDelta, float pitchDelta) noexcept;

	void SetPosition(const DirectX::XMFLOAT3& position) noexcept;
	const TransformComponent& GetTransform() const noexcept { return m_transform; }

	const DirectX::XMFLOAT3& GetDirection() const noexcept;
	DirectX::XMFLOAT3 GetRight() const noexcept;

	void SetYawPitch(float yawRadians, float pitchRadians) noexcept;

	bool IsDirty() const noexcept { return m_dirty; }

	void ClearDirty() noexcept { m_dirty = false; }

	float GetFovYDegrees() const noexcept { return m_fovYDegrees; }
	void SetFovYDegrees(float fovDegrees) noexcept;

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

	TransformComponent m_transform{{0.0f, 0.0f, -4.0f}};

	mutable DirectX::XMFLOAT3 m_cachedDirection = {0.0f, 0.0f, 1.0f};
	mutable bool m_directionDirty = true;

	float m_fovYDegrees = 60.0f;
	float m_nearZ = 0.1f;
	float m_farZ = 1000.0f;
	float m_aspectRatio = 16.0f / 9.0f;

	bool m_dirty = true;
};
