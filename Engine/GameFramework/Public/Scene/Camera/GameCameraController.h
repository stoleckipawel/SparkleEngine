#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include "Events/EventHandle.h"
#include "Events/ScopedEventHandle.h"

class Timer;
class InputSystem;
class Window;
class SceneCamera;
struct KeyboardEvent;
struct MouseButtonEvent;
struct MouseWheelEvent;

class SPARKLE_ENGINE_API GameCameraController final
{
  public:
	GameCameraController(Timer& timer, InputSystem& inputSystem, Window& window, SceneCamera& camera) noexcept;
	~GameCameraController() noexcept;

	GameCameraController(const GameCameraController&) = delete;
	GameCameraController& operator=(const GameCameraController&) = delete;
	GameCameraController(GameCameraController&&) = delete;
	GameCameraController& operator=(GameCameraController&&) = delete;

	void Update() noexcept;

  private:
	void OnMouseButtonPressed(const MouseButtonEvent& event) noexcept;
	void OnMouseButtonReleased(const MouseButtonEvent& event) noexcept;
	void OnKeyPressed(const KeyboardEvent& event) noexcept;
	void OnWindowResized() noexcept;
	void OnMouseWheel(const MouseWheelEvent& event) noexcept;

	Timer& m_timer;
	InputSystem& m_inputSystem;
	Window& m_window;
	SceneCamera& m_camera;

	ScopedEventHandle m_windowResizeHandle;
	EventHandle m_mouseButtonPressedHandle;
	EventHandle m_mouseButtonReleasedHandle;
	EventHandle m_keyPressedHandle;
	EventHandle m_mouseWheelHandle;
	bool m_bMouseLookActive = false;
};