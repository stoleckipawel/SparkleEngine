#include "PCH.h"

#include "Input/CameraInputIntentCollector.h"

#include "Input/Dispatch/InputLayer.h"
#include "Input/Events/MouseMoveEvent.h"
#include "Input/Events/MouseWheelEvent.h"
#include "Input/InputSystem.h"
#include "Input/Keyboard/Key.h"
#include "Input/Mouse/MouseButton.h"
#include "Scene/Camera/CameraInputIntent.h"
#include "Window/Window.h"
#include "World/GameWorld.h"

CameraInputIntentCollector::CameraInputIntentCollector(InputSystem& inputSystem, Window& window) noexcept :
    m_inputSystem(inputSystem), m_window(window)
{
	m_mouseButtonHandle = m_inputSystem.SubscribeMouseButton(
	    [this](const MouseButtonEvent& event) { OnMouseButton(event); }, InputLayer::Gameplay);
	m_mouseMoveHandle = m_inputSystem.SubscribeMouseMove(
	    [this](const MouseMoveEvent& event) { OnMouseMove(event); }, InputLayer::Gameplay);
	m_keyboardHandle = m_inputSystem.SubscribeKeyboard(
	    [this](const KeyboardEvent& event) { OnKeyboard(event); }, InputLayer::Gameplay);
	m_mouseWheelHandle = m_inputSystem.SubscribeMouseWheel(
	    [this](const MouseWheelEvent& event) { OnMouseWheel(event); }, InputLayer::Gameplay);
}

CameraInputIntentCollector::~CameraInputIntentCollector() noexcept
{
	m_inputSystem.Unsubscribe(m_mouseButtonHandle);
	m_inputSystem.Unsubscribe(m_mouseMoveHandle);
	m_inputSystem.Unsubscribe(m_keyboardHandle);
	m_inputSystem.Unsubscribe(m_mouseWheelHandle);
	if (m_mouseLookActive)
	{
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void CameraInputIntentCollector::Publish(GameWorld& world) noexcept
{
	CameraInputIntent intent;
	intent.ForwardAxis = (m_forwardHeld ? 1.0f : 0.0f) - (m_backwardHeld ? 1.0f : 0.0f);
	intent.RightAxis = (m_rightHeld ? 1.0f : 0.0f) - (m_leftHeld ? 1.0f : 0.0f);
	intent.UpAxis = (m_upHeld ? 1.0f : 0.0f) - (m_downHeld ? 1.0f : 0.0f);
	intent.LookDeltaX = m_lookDeltaX;
	intent.LookDeltaY = m_lookDeltaY;
	intent.SpeedStepCount = m_speedStepCount;
	intent.Sprint = m_sprintHeld;
	const float width = static_cast<float>(m_window.GetWidth());
	const float height = static_cast<float>(m_window.GetHeight());
	intent.HasAspectRatio = width > 0.0f && height > 0.0f;
	intent.AspectRatio = intent.HasAspectRatio ? width / height : 1.0f;
	world.PublishCameraInputIntent(intent);
	m_lookDeltaX = 0.0f;
	m_lookDeltaY = 0.0f;
	m_speedStepCount = 0.0f;
}

void CameraInputIntentCollector::OnMouseButton(const MouseButtonEvent& event) noexcept
{
	if (event.Button != MouseButton::Right)
		return;
	if (event.IsPressed() && !m_mouseLookActive)
	{
		m_mouseLookActive = true;
		m_inputSystem.CaptureMouse();
		m_inputSystem.SetCursorVisibility(false);
	}
	else if (event.IsReleased() && m_mouseLookActive)
	{
		m_mouseLookActive = false;
		ResetMovement();
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void CameraInputIntentCollector::OnKeyboard(const KeyboardEvent& event) noexcept
{
	const bool pressed = event.IsPressed();
	switch (event.KeyCode)
	{
		case Key::W: m_forwardHeld = pressed; break;
		case Key::S: m_backwardHeld = pressed; break;
		case Key::D: m_rightHeld = pressed; break;
		case Key::A: m_leftHeld = pressed; break;
		case Key::E:
		case Key::Space: m_upHeld = pressed; break;
		case Key::Q:
		case Key::C: m_downHeld = pressed; break;
		case Key::LeftShift:
		case Key::RightShift: m_sprintHeld = pressed; break;
		case Key::Escape:
			if (pressed && m_mouseLookActive)
			{
				m_mouseLookActive = false;
				ResetMovement();
				m_inputSystem.ReleaseMouse();
				m_inputSystem.SetCursorVisibility(true);
			}
			break;
		default: break;
	}
}

void CameraInputIntentCollector::OnMouseMove(const MouseMoveEvent& event) noexcept
{
	if (!m_mouseLookActive)
		return;
	m_lookDeltaX += static_cast<float>(event.Delta.X);
	m_lookDeltaY += static_cast<float>(event.Delta.Y);
	m_inputSystem.CenterCursor(m_window.GetHWND());
}

void CameraInputIntentCollector::OnMouseWheel(const MouseWheelEvent& event) noexcept
{
	if (event.IsVertical())
		m_speedStepCount += event.Delta;
}

void CameraInputIntentCollector::ResetMovement() noexcept
{
	m_forwardHeld = false;
	m_backwardHeld = false;
	m_rightHeld = false;
	m_leftHeld = false;
	m_upHeld = false;
	m_downHeld = false;
	m_sprintHeld = false;
	m_lookDeltaX = 0.0f;
	m_lookDeltaY = 0.0f;
}
