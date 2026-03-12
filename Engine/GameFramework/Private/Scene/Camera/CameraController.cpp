#include "PCH.h"
#include "CameraController.h"
#include "InputSystem.h"
#include "Camera/GameCamera.h"
#include "Window.h"
#include "Events/ScopedEventHandle.h"
#include "Input/Keyboard/Key.h"
#include "Input/Mouse/MouseButton.h"
#include "Input/Mouse/MousePosition.h"
#include "Input/Events/MouseWheelEvent.h"
#include "Timer.h"
#include <algorithm>

CameraController::CameraController(Timer& timer, InputSystem& inputSystem, Window& window, GameCamera& camera) noexcept :
    m_timer(timer), m_inputSystem(inputSystem), m_window(window), m_camera(camera)
{
	OnWindowResized();

	auto resizeHandle = m_window.OnResized.Add(
	    [this]()
	    {
		    OnWindowResized();
	    });
	m_windowResizeHandle = ScopedEventHandle(m_window.OnResized, resizeHandle);

	auto mouseButtonPressedHandle = m_inputSystem.OnMouseButtonPressed.Add(
	    [this](const MouseButtonEvent& event)
	    {
		    OnMouseButtonPressed(event);
	    });
	m_mouseButtonPressedHandle = ScopedEventHandle(m_inputSystem.OnMouseButtonPressed, mouseButtonPressedHandle);

	auto mouseButtonReleasedHandle = m_inputSystem.OnMouseButtonReleased.Add(
	    [this](const MouseButtonEvent& event)
	    {
		    OnMouseButtonReleased(event);
	    });
	m_mouseButtonReleasedHandle = ScopedEventHandle(m_inputSystem.OnMouseButtonReleased, mouseButtonReleasedHandle);

	auto keyPressedHandle = m_inputSystem.OnKeyPressed.Add(
	    [this](const KeyboardEvent& event)
	    {
		    OnKeyPressed(event);
	    });
	m_keyPressedHandle = ScopedEventHandle(m_inputSystem.OnKeyPressed, keyPressedHandle);

	auto mouseWheelHandle = m_inputSystem.OnMouseWheel.Add(
	    [this](const MouseWheelEvent& event)
	    {
		    OnMouseWheel(event);
	    });
	m_mouseWheelHandle = ScopedEventHandle(m_inputSystem.OnMouseWheel, mouseWheelHandle);
}

CameraController::~CameraController() noexcept
{
	if (m_bMouseLookActive)
	{
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void CameraController::Update() noexcept
{
	const InputState& input = m_inputSystem.GetState();
	const float deltaTime = static_cast<float>(m_timer.GetDelta(TimeDomain::Scaled));

	if (m_bMouseLookActive)
	{
		const MousePosition mouseDelta = input.GetMouseDelta();

		const float ySign = m_settings.invertY ? 1.0f : -1.0f;

		const float yawDelta = static_cast<float>(mouseDelta.X) * m_settings.mouseSensitivity;
		const float pitchDelta = ySign * static_cast<float>(mouseDelta.Y) * m_settings.mouseSensitivity;

		m_camera.Rotate(yawDelta, pitchDelta);

		m_inputSystem.CenterCursor(m_window.GetHWND());
	}

	if (deltaTime <= 0.0f)
	{
		return;
	}

	float speed = m_settings.moveSpeed;
	if (input.IsKeyDown(Key::LeftShift) || input.IsKeyDown(Key::RightShift))
	{
		speed *= m_settings.sprintMultiplier;
	}

	const float distance = speed * deltaTime;

	if (input.IsKeyDown(Key::W))
		m_camera.MoveForward(distance);
	if (input.IsKeyDown(Key::S))
		m_camera.MoveForward(-distance);
	if (input.IsKeyDown(Key::D))
		m_camera.MoveRight(distance);
	if (input.IsKeyDown(Key::A))
		m_camera.MoveRight(-distance);
	if (input.IsKeyDown(Key::E) || input.IsKeyDown(Key::Space))
		m_camera.MoveUp(distance);
	if (input.IsKeyDown(Key::Q) || input.IsKeyDown(Key::C))
		m_camera.MoveUp(-distance);
}

void CameraController::OnMouseButtonPressed(const MouseButtonEvent& event) noexcept
{
	if (event.Button == MouseButton::Right)
	{
		m_bMouseLookActive = true;
		m_inputSystem.CaptureMouse();
		m_inputSystem.SetCursorVisibility(false);
	}
}

void CameraController::OnMouseButtonReleased(const MouseButtonEvent& event) noexcept
{
	if (event.Button == MouseButton::Right)
	{
		m_bMouseLookActive = false;
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void CameraController::OnKeyPressed(const KeyboardEvent& event) noexcept
{
	if (event.KeyCode == Key::Escape && m_bMouseLookActive)
	{
		m_bMouseLookActive = false;
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void CameraController::OnWindowResized() noexcept
{
	const float width = static_cast<float>(m_window.GetWidth());
	const float height = static_cast<float>(m_window.GetHeight());
	if (width > 0.0f && height > 0.0f)
	{
		m_camera.SetAspectRatio(width / height);
	}
}

void CameraController::OnMouseWheel(const MouseWheelEvent& event) noexcept
{
	if (!event.IsVertical())
	{
		return;
	}

	m_settings.moveSpeed += event.Delta * m_settings.speedStep;
	m_settings.moveSpeed = std::clamp(m_settings.moveSpeed, m_settings.minMoveSpeed, m_settings.maxMoveSpeed);
}
