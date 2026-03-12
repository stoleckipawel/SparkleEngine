#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

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
	float minMoveSpeed = 0.01f;
	float maxMoveSpeed = 10.0f;
	float speedStep = 0.1f;
	float sprintMultiplier = 2.0f;
	float mouseSensitivity = 0.0015f;
	bool invertY = false;
};

class SPARKLE_ENGINE_API CameraController final
{
  public:
	CameraController(Timer& timer, InputSystem& inputSystem, Window& window, GameCamera& camera) noexcept;
	~CameraController() noexcept;

	CameraController(const CameraController&) = delete;
	CameraController& operator=(const CameraController&) = delete;
	CameraController(CameraController&&) = delete;
	CameraController& operator=(CameraController&&) = delete;

	void Update() noexcept;

	CameraMovementSettings& GetSettings() noexcept { return m_settings; }
	const CameraMovementSettings& GetSettings() const noexcept { return m_settings; }

	void SetSettings(const CameraMovementSettings& settings) noexcept { m_settings = settings; }

	void SetMoveSpeed(float speed) noexcept { m_settings.moveSpeed = speed; }
	void SetSprintMultiplier(float multiplier) noexcept { m_settings.sprintMultiplier = multiplier; }
	void SetMouseSensitivity(float sensitivity) noexcept { m_settings.mouseSensitivity = sensitivity; }
	void SetInvertY(bool invert) noexcept { m_settings.invertY = invert; }

	float GetMoveSpeed() const noexcept { return m_settings.moveSpeed; }
	float GetSprintMultiplier() const noexcept { return m_settings.sprintMultiplier; }
	float GetMouseSensitivity() const noexcept { return m_settings.mouseSensitivity; }
	bool GetInvertY() const noexcept { return m_settings.invertY; }

  private:
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
