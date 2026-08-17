#include "PCH.h"

#include "Input/CameraInputIntentCollector.h"

#include "Input/Dispatch/InputLayer.h"
#include "Input/Events/MouseMoveEvent.h"
#include "Input/Events/MouseWheelEvent.h"
#include "Input/InputState.h"
#include "Input/InputSystem.h"
#include "Input/Keyboard/Key.h"
#include "Input/Mouse/MouseButton.h"
#include "Scene/Camera/CameraInputIntent.h"
#include "Window/Window.h"
#include "World/GameWorld.h"

CameraInputIntentCollector::CameraInputIntentCollector(InputSystem& inputSystem, Window& window) noexcept :
    m_inputSystem(inputSystem),
    m_window(window)
{
	m_mouseButtonHandle =
	    m_inputSystem.SubscribeMouseButton([this](const MouseButtonEvent& event) { OnMouseButton(event); }, InputLayer::Gameplay);
	m_mouseMoveHandle = m_inputSystem.SubscribeMouseMove([this](const MouseMoveEvent& event) { OnMouseMove(event); }, InputLayer::Gameplay);
	m_keyboardHandle = m_inputSystem.SubscribeKeyboard([this](const KeyboardEvent& event) { OnKeyboard(event); }, InputLayer::Gameplay);
	m_mouseWheelHandle =
	    m_inputSystem.SubscribeMouseWheel([this](const MouseWheelEvent& event) { OnMouseWheel(event); }, InputLayer::Gameplay);
}

CameraInputIntentCollector::~CameraInputIntentCollector() noexcept
{
	m_inputSystem.Unsubscribe(m_mouseButtonHandle);
	m_inputSystem.Unsubscribe(m_mouseMoveHandle);
	m_inputSystem.Unsubscribe(m_keyboardHandle);
	m_inputSystem.Unsubscribe(m_mouseWheelHandle);
	EndMouseLook();
}

void CameraInputIntentCollector::Publish(GameWorld& world) noexcept
{
	const InputState& inputState = m_inputSystem.GetState(InputLayer::Gameplay);
	const bool forwardHeld = inputState.IsKeyDown(Key::W) || inputState.IsKeyDown(Key::Up);
	const bool backwardHeld = inputState.IsKeyDown(Key::S) || inputState.IsKeyDown(Key::Down);
	const bool rightHeld = inputState.IsKeyDown(Key::D) || inputState.IsKeyDown(Key::Right);
	const bool leftHeld = inputState.IsKeyDown(Key::A) || inputState.IsKeyDown(Key::Left);
	const bool upHeld = inputState.IsKeyDown(Key::E) || inputState.IsKeyDown(Key::Space);
	const bool downHeld = inputState.IsKeyDown(Key::Q) || inputState.IsKeyDown(Key::C);

	CameraInputIntent intent;
	intent.ForwardAxis = (forwardHeld ? 1.0f : 0.0f) - (backwardHeld ? 1.0f : 0.0f);
	intent.RightAxis = (rightHeld ? 1.0f : 0.0f) - (leftHeld ? 1.0f : 0.0f);
	intent.UpAxis = (upHeld ? 1.0f : 0.0f) - (downHeld ? 1.0f : 0.0f);
	intent.LookDeltaX = m_lookDeltaX;
	intent.LookDeltaY = m_lookDeltaY;
	intent.SpeedStepCount = m_speedStepCount;
	intent.Sprint = inputState.IsKeyDown(Key::LeftShift) || inputState.IsKeyDown(Key::RightShift);
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
		EndMouseLook();
	}
}

void CameraInputIntentCollector::OnKeyboard(const KeyboardEvent& event) noexcept
{
	if (event.KeyCode == Key::Escape && event.IsPressed())
	{
		EndMouseLook();
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

void CameraInputIntentCollector::EndMouseLook() noexcept
{
	if (!m_mouseLookActive)
		return;

	m_mouseLookActive = false;
	m_lookDeltaX = 0.0f;
	m_lookDeltaY = 0.0f;
	m_inputSystem.ReleaseMouse();
	m_inputSystem.SetCursorVisibility(true);
}
