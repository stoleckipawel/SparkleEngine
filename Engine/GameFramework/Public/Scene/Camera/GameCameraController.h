#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"

#include "Events/ScopedEventHandle.h"

class Timer;
class InputSystem;
class Window;
class GameCamera;
struct KeyboardEvent;
struct MouseButtonEvent;
struct MouseWheelEvent;

struct CameraMovementSettings
{
	float moveSpeed = 0.15f;
	float minMoveSpeed = 0.0001f;
	float maxMoveSpeed = 10.0f;
	float speedStep = 0.01f;
	float sprintMultiplier = 2.0f;
	float mouseSensitivity = 0.0015f;
	bool invertY = false;
};

class SPARKLE_ENGINE_API GameCameraController final
{
  public:
	GameCameraController(Timer& timer, InputSystem& inputSystem, Window& window, GameCamera& camera) noexcept;
	~GameCameraController() noexcept;

	GameCameraController(const GameCameraController&) = delete;
	GameCameraController& operator=(const GameCameraController&) = delete;
	GameCameraController(GameCameraController&&) = delete;
	GameCameraController& operator=(GameCameraController&&) = delete;

	void Update() noexcept;

	CameraMovementSettings& GetSettings() noexcept { return m_settings; }
	const CameraMovementSettings& GetSettings() const noexcept { return m_settings; }

	void SetSettings(const CameraMovementSettings& settings) noexcept;
	void ApplyCameraDesc(const CameraDesc& cameraDesc) noexcept;
	CameraDesc CaptureCurrentCameraDesc() const noexcept;

	void SetPosition(const DirectX::XMFLOAT3& position) noexcept;
	void SetYaw(float yawRadians) noexcept;
	void SetPitch(float pitchRadians) noexcept;
	void SetYawPitch(float yawRadians, float pitchRadians) noexcept;
	void SetMoveSpeed(float speed) noexcept;
	void SetSprintMultiplier(float multiplier) noexcept { m_settings.sprintMultiplier = multiplier; }
	void SetMouseSensitivity(float sensitivity) noexcept { m_settings.mouseSensitivity = sensitivity; }
	void SetInvertY(bool invert) noexcept { m_settings.invertY = invert; }
	void SetFovYDegrees(float fovDegrees) noexcept;

	DirectX::XMFLOAT3 GetPosition() const noexcept;
	float GetYaw() const noexcept;
	float GetPitch() const noexcept;
	float GetMoveSpeed() const noexcept { return m_settings.moveSpeed; }
	float GetSprintMultiplier() const noexcept { return m_settings.sprintMultiplier; }
	float GetMouseSensitivity() const noexcept { return m_settings.mouseSensitivity; }
	bool GetInvertY() const noexcept { return m_settings.invertY; }
	float GetFovYDegrees() const noexcept;

  private:
	static float ClampPitch(float pitchRadians) noexcept;
	float ClampMoveSpeed(float speed) const noexcept;
	float ClampFovYDegrees(float fovDegrees) const noexcept;

	void OnMouseButtonPressed(const MouseButtonEvent& event) noexcept;
	void OnMouseButtonReleased(const MouseButtonEvent& event) noexcept;
	void OnKeyPressed(const KeyboardEvent& event) noexcept;
	void OnWindowResized() noexcept;
	void OnMouseWheel(const MouseWheelEvent& event) noexcept;

	Timer& m_timer;
	InputSystem& m_inputSystem;
	Window& m_window;
	GameCamera& m_camera;

	ScopedEventHandle m_mouseButtonPressedHandle;
	ScopedEventHandle m_mouseButtonReleasedHandle;
	ScopedEventHandle m_keyPressedHandle;
	ScopedEventHandle m_windowResizeHandle;
	ScopedEventHandle m_mouseWheelHandle;

	CameraMovementSettings m_settings;
	bool m_bMouseLookActive = false;
};