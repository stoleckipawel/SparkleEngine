#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraMovementSettings.h"
#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Camera/CameraComponent.h"

class SPARKLE_ENGINE_API SceneCamera final
{
  public:
	SceneCamera() = default;
	~SceneCamera() noexcept = default;

	SceneCamera(const SceneCamera&) = delete;
	SceneCamera& operator=(const SceneCamera&) = delete;
	SceneCamera(SceneCamera&&) = delete;
	SceneCamera& operator=(SceneCamera&&) = delete;

	CameraComponent& GetCameraComponent() noexcept { return m_camera; }
	const CameraComponent& GetCameraComponent() const noexcept { return m_camera; }

	const CameraMovementSettings& GetSettings() const noexcept { return m_settings; }
	void SetSettings(const CameraMovementSettings& settings) noexcept;

	void ApplyFromDesc(const CameraDesc& desc) noexcept;
	CameraDesc CaptureToDesc() const noexcept;
	CameraSnapshot CaptureSnapshot() const noexcept;

  private:
	float ClampMoveSpeed(float speed) const noexcept;

	CameraComponent m_camera;
	CameraMovementSettings m_settings;
};
