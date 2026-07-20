#pragma once

#include "Events/EventHandle.h"

class GameWorld;
class InputSystem;
class Window;
struct KeyboardEvent;
struct MouseButtonEvent;
struct MouseMoveEvent;
struct MouseWheelEvent;

class CameraInputIntentCollector final
{
  public:
	CameraInputIntentCollector(InputSystem& inputSystem, Window& window) noexcept;
	~CameraInputIntentCollector() noexcept;

	CameraInputIntentCollector(const CameraInputIntentCollector&) = delete;
	CameraInputIntentCollector& operator=(const CameraInputIntentCollector&) = delete;
	void Publish(GameWorld& world) noexcept;

  private:
	void OnMouseButton(const MouseButtonEvent& event) noexcept;
	void OnKeyboard(const KeyboardEvent& event) noexcept;
	void OnMouseMove(const MouseMoveEvent& event) noexcept;
	void OnMouseWheel(const MouseWheelEvent& event) noexcept;
	void ResetMovement() noexcept;

	InputSystem& m_inputSystem;
	Window& m_window;
	EventHandle m_mouseButtonHandle;
	EventHandle m_mouseMoveHandle;
	EventHandle m_keyboardHandle;
	EventHandle m_mouseWheelHandle;
	float m_lookDeltaX = 0.0f;
	float m_lookDeltaY = 0.0f;
	float m_speedStepCount = 0.0f;
	bool m_forwardHeld = false;
	bool m_backwardHeld = false;
	bool m_rightHeld = false;
	bool m_leftHeld = false;
	bool m_upHeld = false;
	bool m_downHeld = false;
	bool m_sprintHeld = false;
	bool m_mouseLookActive = false;
};
