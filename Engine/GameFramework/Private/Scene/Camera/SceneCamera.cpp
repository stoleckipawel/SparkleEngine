#include "PCH.h"
#include "Scene/Camera/SceneCamera.h"

#include <algorithm>

void SceneCamera::SetSettings(const CameraMovementSettings& settings) noexcept
{
	m_settings = settings;
	m_settings.moveSpeed = ClampMoveSpeed(settings.moveSpeed);
}

void SceneCamera::ApplyFromDesc(const CameraDesc& desc) noexcept
{
	m_camera.SetPosition(desc.position);
	m_camera.SetYawPitch(desc.yawRadians, desc.pitchRadians);
	m_camera.SetFovYDegrees(desc.fovYDegrees);
	m_camera.SetNearFar(desc.nearZ, desc.farZ);
	m_settings.moveSpeed = ClampMoveSpeed(desc.moveSpeed);
}

CameraDesc SceneCamera::CaptureToDesc() const noexcept
{
	CameraDesc desc;
	const Transform& cameraTransform = m_camera.GetTransform();
	const DirectX::XMFLOAT3 rotationEuler = cameraTransform.GetRotationEuler();
	desc.position = cameraTransform.GetTranslation();
	desc.yawRadians = rotationEuler.y;
	desc.pitchRadians = rotationEuler.x;
	desc.fovYDegrees = m_camera.GetFovYDegrees();
	desc.nearZ = m_camera.GetNearZ();
	desc.farZ = m_camera.GetFarZ();
	desc.moveSpeed = m_settings.moveSpeed;
	return desc;
}

CameraSnapshot SceneCamera::CaptureSnapshot() const noexcept
{
	CameraSnapshot snapshot;
	snapshot.position = m_camera.GetTransform().GetTranslation();
	snapshot.direction = m_camera.GetDirection();
	snapshot.fovYDegrees = m_camera.GetFovYDegrees();
	snapshot.aspectRatio = m_camera.GetAspectRatio();
	snapshot.nearZ = m_camera.GetNearZ();
	snapshot.farZ = m_camera.GetFarZ();
	return snapshot;
}

float SceneCamera::ClampMoveSpeed(float speed) const noexcept
{
	return std::clamp(speed, m_settings.minMoveSpeed, m_settings.maxMoveSpeed);
}
