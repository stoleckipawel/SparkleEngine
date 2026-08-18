#pragma once

#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraNavigationSettings.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <filesystem>

struct EditorViewportSettingsState final
{
	CameraNavigationSettings Navigation;
	CameraProjectionKind ProjectionKind = CameraProjectionKind::Perspective;
	float OrthographicHeightMeters = 10.0f;
	ViewportExposureOverrides Exposure;
};

class EditorViewportSettings final
{
public:
	EditorViewportSettings();
	explicit EditorViewportSettings(std::filesystem::path path);

	const EditorViewportSettingsState& GetState() const noexcept { return m_state; }
	bool SetMoveSpeed(float speedMetersPerSecond) noexcept;
	bool SetRotationSpeed(float degreesPerPixel) noexcept;
	bool SetInvertY(bool invertY) noexcept;
	bool SetProjectionKind(CameraProjectionKind projectionKind) noexcept;
	bool SetOrthographicHeight(float heightMeters) noexcept;
	bool SetExposureOverrides(ViewportExposureOverrides overrides) noexcept;
	bool Reload() noexcept;

	static std::filesystem::path GetDefaultPath();

private:
	static void Sanitize(EditorViewportSettingsState& state) noexcept;
	static void SanitizeExposure(ViewportExposureOverrides& exposure) noexcept;
	bool Save() const noexcept;

	std::filesystem::path m_path;
	EditorViewportSettingsState m_state;
};
