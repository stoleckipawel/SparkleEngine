#include "PCH.h"

#include "Viewport/EditorViewportSession.h"

#include <utility>

EditorViewportSession::EditorViewportSession() = default;

EditorViewportSession::EditorViewportSession(EditorViewportSettings settings) :
    m_settings(std::move(settings))
{
}

void EditorViewportSession::SynchronizeWorld(std::span<const WorldCameraReadData> cameras, std::uint64_t worldGeneration) noexcept
{
	if (m_cameraInitialized && m_worldGeneration == worldGeneration)
	{
		return;
	}

	const WorldCameraReadData* source = nullptr;
	for (const WorldCameraReadData& camera : cameras)
	{
		if (camera.Active)
		{
			source = &camera;
			break;
		}
	}
	if (source == nullptr && !cameras.empty())
	{
		source = &cameras.front();
	}

	if (source != nullptr)
	{
		const DirectX::XMFLOAT3 rotation = source->LocalTransform.GetRotationEuler();
		m_navigationState.Position = source->LocalTransform.GetTranslation();
		m_navigationState.PitchRadians = rotation.x;
		m_navigationState.YawRadians = rotation.y;
		m_camera.FovYDegrees = source->Description.fovYDegrees;
		m_camera.NearZ = source->Description.nearZ;
		m_camera.FarZ = source->Description.farZ;
	}
	m_worldGeneration = worldGeneration;
	m_cameraInitialized = true;
}

RenderViewCameraData EditorViewportSession::UpdateCamera(
    const CameraInputIntent& intent,
    float deltaSeconds,
    RenderViewportExtent extent) noexcept
{
	if (intent.SpeedStepCount != 0.0f)
	{
		const CameraNavigationSettings& navigation = m_settings.GetState().Navigation;
		(void) m_settings.SetMoveSpeed(
		    CameraNavigation::ApplySpeedSteps(
		        navigation.MoveSpeedMetersPerSecond,
		        intent.SpeedStepCount,
		        navigation.MinimumMoveSpeedMetersPerSecond,
		        navigation.MaximumMoveSpeedMetersPerSecond));
	}

	(void) CameraNavigation::Apply(intent, m_settings.GetState().Navigation, deltaSeconds, m_navigationState);
	m_camera.Position = m_navigationState.Position;
	m_camera.Direction = CameraNavigation::BuildDirection(m_navigationState);
	if (extent.IsValid())
	{
		m_camera.AspectRatio = static_cast<float>(extent.Width) / static_cast<float>(extent.Height);
	}
	m_camera.ProjectionKind = m_settings.GetState().ProjectionKind;
	m_camera.OrthographicHeightMeters = m_settings.GetState().OrthographicHeightMeters;
	return m_camera;
}

void EditorViewportSession::SetMoveSpeed(float speedMetersPerSecond) noexcept
{
	(void) m_settings.SetMoveSpeed(speedMetersPerSecond);
}

void EditorViewportSession::SetRotationSpeed(float degreesPerPixel) noexcept
{
	(void) m_settings.SetRotationSpeed(degreesPerPixel);
}

void EditorViewportSession::SetInvertY(bool invertY) noexcept
{
	(void) m_settings.SetInvertY(invertY);
}

void EditorViewportSession::SetProjectionKind(CameraProjectionKind projectionKind) noexcept
{
	(void) m_settings.SetProjectionKind(projectionKind);
}

void EditorViewportSession::SetOrthographicHeight(float heightMeters) noexcept
{
	(void) m_settings.SetOrthographicHeight(heightMeters);
}

void EditorViewportSession::SetExposureOverrides(ViewportExposureOverrides overrides) noexcept
{
	(void) m_settings.SetExposureOverrides(overrides);
}
