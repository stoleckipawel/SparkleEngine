#include "PCH.h"

#include "Scene/TransformComponent.h"

#include <algorithm>
#include <cmath>

TransformComponent::TransformComponent() noexcept = default;

TransformComponent::~TransformComponent() = default;

TransformComponent::TransformComponent(
	const DirectX::XMFLOAT3& translation,
	const DirectX::XMFLOAT3& rotationEuler,
	const DirectX::XMFLOAT3& scale) noexcept
	: m_translation(translation), m_rotationEuler(rotationEuler), m_scale(scale)
{
}

void TransformComponent::SetTranslation(const DirectX::XMFLOAT3& translation) noexcept
{
	m_translation = translation;
	InvalidateCache();
}

DirectX::XMFLOAT3 TransformComponent::GetTranslation() const noexcept
{
	return m_translation;
}

void TransformComponent::Translate(const DirectX::XMFLOAT3& delta) noexcept
{
	m_translation.x += delta.x;
	m_translation.y += delta.y;
	m_translation.z += delta.z;
	InvalidateCache();
}

void TransformComponent::TranslateScaled(const DirectX::XMFLOAT3& direction, float distance) noexcept
{
	DirectX::XMFLOAT3 position = m_translation;
	DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
	pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(dir, distance));
	DirectX::XMStoreFloat3(&position, pos);
	SetTranslation(position);
}

void TransformComponent::SetRotationEuler(const DirectX::XMFLOAT3& rotationEuler) noexcept
{
	m_rotationEuler = rotationEuler;
	InvalidateCache();
}

DirectX::XMFLOAT3 TransformComponent::GetRotationEuler() const noexcept
{
	return m_rotationEuler;
}

void TransformComponent::RotateEuler(const DirectX::XMFLOAT3& deltaEuler) noexcept
{
	m_rotationEuler.x += deltaEuler.x;
	m_rotationEuler.y += deltaEuler.y;
	m_rotationEuler.z += deltaEuler.z;
	InvalidateCache();
}

void TransformComponent::RotateYawPitch(float yawDelta, float pitchDelta, float minPitch, float maxPitch) noexcept
{
	DirectX::XMFLOAT3 rotationEuler = m_rotationEuler;
	rotationEuler.y += yawDelta;
	rotationEuler.x += pitchDelta;
	SetYawPitch(rotationEuler.y, rotationEuler.x, minPitch, maxPitch);
}

void TransformComponent::SetYawPitch(float yaw, float pitch, float minPitch, float maxPitch) noexcept
{
	DirectX::XMFLOAT3 rotationEuler = m_rotationEuler;
	rotationEuler.x = std::clamp(pitch, minPitch, maxPitch);
	rotationEuler.y = std::fmod(yaw, DirectX::XM_2PI);
	if (rotationEuler.y < 0.0f)
	{
		rotationEuler.y += DirectX::XM_2PI;
	}

	SetRotationEuler(rotationEuler);
}

void TransformComponent::SetScale(const DirectX::XMFLOAT3& scale) noexcept
{
	m_scale = scale;
	InvalidateCache();
}

DirectX::XMFLOAT3 TransformComponent::GetScale() const noexcept
{
	return m_scale;
}

void TransformComponent::InvalidateCache() noexcept
{
	m_bWorldDirty = true;
}

DirectX::XMMATRIX TransformComponent::GetWorldMatrix() const noexcept
{
	RebuildWorldIfNeeded();
	return DirectX::XMLoadFloat4x4(&m_worldMatrixCache);
}

DirectX::XMMATRIX TransformComponent::GetWorldInverseTransposeMatrix() const noexcept
{
	const DirectX::XMMATRIX world = GetWorldMatrix();
	return DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, world));
}

DirectX::XMFLOAT3X3 TransformComponent::GetRotationMatrix3x3() const noexcept
{
	const DirectX::XMFLOAT3 rotationEuler = m_rotationEuler;
	const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(rotationEuler.x, rotationEuler.y, rotationEuler.z);

	DirectX::XMFLOAT3X3 rotation3x3;
	DirectX::XMStoreFloat3x3(&rotation3x3, rotation);
	return rotation3x3;
}

const DirectX::XMFLOAT4X4& TransformComponent::GetWorldMatrix4x4() const noexcept
{
	RebuildWorldIfNeeded();
	return m_worldMatrixCache;
}

void TransformComponent::RebuildWorldIfNeeded() const noexcept
{
	if (!m_bWorldDirty)
	{
		return;
	}

	const DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(m_rotationEuler.x, m_rotationEuler.y, m_rotationEuler.z);
	const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);

	DirectX::XMStoreFloat4x4(&m_worldMatrixCache, scale * rotation * translation);
	m_bWorldDirty = false;
}