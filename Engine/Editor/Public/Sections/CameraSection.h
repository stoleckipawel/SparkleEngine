#pragma once

#include "Framework/UIRendererSection.h"

#include <string>

class SceneCamera;

class CameraSection final : public UIRendererSection
{
  public:
	explicit CameraSection(SceneCamera& sceneCamera) noexcept;
	~CameraSection() = default;

	CameraSection(const CameraSection&) = delete;
	CameraSection(CameraSection&&) = delete;
	CameraSection& operator=(const CameraSection&) = delete;
	CameraSection& operator=(CameraSection&&) = delete;

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::Camera; }
	const char* GetTitle() const noexcept override { return "Camera"; }

	void BuildUI() override;

  private:
	static void ClampCameraUiValues(float& pitchDegrees, float& fovYDegrees, float& moveSpeed) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kYawSliderMin = -360.0f;
	static constexpr float kYawSliderMax = 360.0f;

	SceneCamera* m_sceneCamera = nullptr;
};
