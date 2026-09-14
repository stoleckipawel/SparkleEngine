#pragma once

#include "GameFramework/Public/Rendering/RenderViewCameraData.h"
#include "GameFramework/Public/Scene/Camera/CameraNavigation.h"
#include "Editor/Public/Viewport/EditorViewportViewMode.h"
#include "Viewport/EditorViewportSettings.h"
#include "World/WorldReadView.h"

#include <cstdint>
#include <functional>
#include <span>

class EditorViewportSession final
{
public:
	EditorViewportSession();
	explicit EditorViewportSession(EditorViewportSettings settings);
	void SetViewModeChangedHandler(std::function<void(EditorViewportViewMode)> handler) noexcept;

	void SynchronizeWorld(std::span<const WorldCameraReadData> cameras, std::uint64_t worldGeneration) noexcept;
	RenderViewCameraData UpdateCamera(const CameraInputIntent& intent, float deltaSeconds, RenderViewportExtent extent) noexcept;

	const EditorViewportSettingsState& GetSettings() const noexcept { return m_settings.GetState(); }
	EditorViewportViewMode GetViewMode() const noexcept { return m_viewMode; }
	void SetMoveSpeed(float speedMetersPerSecond) noexcept;
	void SetRotationSpeed(float degreesPerPixel) noexcept;
	void SetInvertY(bool invertY) noexcept;
	void SetProjectionKind(CameraProjectionKind projectionKind) noexcept;
	void SetOrthographicHeight(float heightMeters) noexcept;
	void SetExposureOverrides(ViewportExposureOverrides overrides) noexcept;
	void SetViewMode(EditorViewportViewMode viewMode) noexcept;

private:
	EditorViewportSettings m_settings;
	CameraNavigationState m_navigationState;
	RenderViewCameraData m_camera;
	std::function<void(EditorViewportViewMode)> m_viewModeChangedHandler;
	EditorViewportViewMode m_viewMode = EditorViewportViewMode::Lit;
	std::uint64_t m_worldGeneration = 0;
	bool m_cameraInitialized = false;
};
