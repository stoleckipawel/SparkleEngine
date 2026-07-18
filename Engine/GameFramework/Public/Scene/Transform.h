#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

class SPARKLE_ENGINE_API Transform
{
  public:
	Transform() noexcept;
	explicit Transform(const DirectX::XMMATRIX& worldTransform) noexcept;
	Transform(
	    const DirectX::XMFLOAT3& translation,
	    const DirectX::XMFLOAT3& rotationEuler = {0.0f, 0.0f, 0.0f},
	    const DirectX::XMFLOAT3& scale = {1.0f, 1.0f, 1.0f}) noexcept;

	void SetTranslation(const DirectX::XMFLOAT3& translation) noexcept;
	DirectX::XMFLOAT3 GetTranslation() const noexcept;

	void Translate(const DirectX::XMFLOAT3& delta) noexcept;
	void TranslateScaled(const DirectX::XMFLOAT3& direction, float distance) noexcept;

	void SetRotationEuler(const DirectX::XMFLOAT3& rotationEuler) noexcept;
	DirectX::XMFLOAT3 GetRotationEuler() const noexcept;

	void RotateEuler(const DirectX::XMFLOAT3& deltaEuler) noexcept;
	void RotateYawPitch(float yawDelta, float pitchDelta, float minPitch, float maxPitch) noexcept;
	void SetYawPitch(float yaw, float pitch, float minPitch, float maxPitch) noexcept;

	void SetScale(const DirectX::XMFLOAT3& scale) noexcept;
	DirectX::XMFLOAT3 GetScale() const noexcept;

	DirectX::XMMATRIX GetWorldMatrix() const noexcept;
	DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept;
	DirectX::XMFLOAT3X3 GetRotationMatrix3x3() const noexcept;

  private:
	DirectX::XMFLOAT3 m_translation{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_rotationEuler{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 m_scale{1.0f, 1.0f, 1.0f};
};
