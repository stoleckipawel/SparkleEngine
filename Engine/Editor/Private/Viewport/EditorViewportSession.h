#pragma once

#include "GameFramework/Public/Rendering/RenderViewCameraData.h"
#include "GameFramework/Public/Scene/Camera/CameraNavigation.h"
#include "Viewport/EditorViewportSettings.h"
#include "World/WorldReadView.h"

#include <cstdint>
#include <span>

class EditorViewportSession final
{
public:
	EditorViewportSession();
	explicit EditorViewportSession(EditorViewportSettings settings);

	void SynchronizeWorld(std::span<const WorldCameraReadData> cameras, std::uint64_t worldGeneration) noexcept;
	RenderViewCameraData UpdateCamera(const CameraInputIntent& intent, float deltaSeconds, RenderViewportExtent extent) noexcept;

	const EditorViewportSettingsState& GetSettings() const noexcept { return m_settings.GetState(); }
	void SetMoveSpeed(float speedMetersPerSecond) noexcept;
	void SetRotationSpeed(float degreesPerPixel) noexcept;
	void SetInvertY(bool invertY) noexcept;
	void SetProjectionKind(CameraProjectionKind projectionKind) noexcept;
	void SetOrthographicHeight(float heightMeters) noexcept;
	void SetExposureOverrides(ViewportExposureOverrides overrides) noexcept;

private:
	EditorViewportSettings m_settings;
	CameraNavigationState m_navigationState;
	RenderViewCameraData m_camera;
	std::uint64_t m_worldGeneration = 0;
	bool m_cameraInitialized = false;
};
