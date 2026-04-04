#include "PCH.h"
#include "Scene/Camera/GameCameraController.h"
#include "Input/InputSystem.h"
#include "Scene/Camera/SceneCamera.h"
#include "Window/Window.h"
#include "Events/ScopedEventHandle.h"
#include "Input/Keyboard/Key.h"
#include "Input/Mouse/MouseButton.h"
#include "Input/Mouse/MousePosition.h"
#include "Input/Events/MouseWheelEvent.h"
#include "Timer.h"

#include <DirectXMath.h>

GameCameraController::GameCameraController(Timer& timer, InputSystem& inputSystem, Window& window, SceneCamera& camera) noexcept :
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

GameCameraController::~GameCameraController() noexcept
{
	if (m_bMouseLookActive)
	{
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void GameCameraController::Update() noexcept
{
	const InputState& input = m_inputSystem.GetState();
	const float deltaTime = static_cast<float>(m_timer.GetDelta(TimeDomain::Scaled));
	CameraComponent& cameraComponent = m_camera.GetCameraComponent();
	const CameraMovementSettings& settings = m_camera.GetSettings();

	if (m_bMouseLookActive)
	{
		const MousePosition mouseDelta = input.GetMouseDelta();
		const DirectX::XMFLOAT3 rotationEuler = cameraComponent.GetTransform().GetRotationEuler();

		const float ySign = settings.invertY ? 1.0f : -1.0f;

		const float yawDelta = static_cast<float>(mouseDelta.X) * settings.mouseSensitivity;
		const float pitchDelta = ySign * static_cast<float>(mouseDelta.Y) * settings.mouseSensitivity;

		cameraComponent.SetYawPitch(rotationEuler.y + yawDelta, rotationEuler.x + pitchDelta);

		m_inputSystem.CenterCursor(m_window.GetHWND());
	}

	if (deltaTime <= 0.0f)
	{
		return;
	}

	float speed = settings.moveSpeed;
	if (input.IsKeyDown(Key::LeftShift) || input.IsKeyDown(Key::RightShift))
	{
		speed *= settings.sprintMultiplier;
	}

	const float distance = speed * deltaTime;

	if (input.IsKeyDown(Key::W))
		cameraComponent.MoveForward(distance);
	if (input.IsKeyDown(Key::S))
		cameraComponent.MoveForward(-distance);
	if (input.IsKeyDown(Key::D))
		cameraComponent.MoveRight(distance);
	if (input.IsKeyDown(Key::A))
		cameraComponent.MoveRight(-distance);
	if (input.IsKeyDown(Key::E) || input.IsKeyDown(Key::Space))
		cameraComponent.MoveUp(distance);
	if (input.IsKeyDown(Key::Q) || input.IsKeyDown(Key::C))
		cameraComponent.MoveUp(-distance);
}

void GameCameraController::OnMouseButtonPressed(const MouseButtonEvent& event) noexcept
{
	if (event.Button == MouseButton::Right)
	{
		m_bMouseLookActive = true;
		m_inputSystem.CaptureMouse();
		m_inputSystem.SetCursorVisibility(false);
	}
}

void GameCameraController::OnMouseButtonReleased(const MouseButtonEvent& event) noexcept
{
	if (event.Button == MouseButton::Right)
	{
		m_bMouseLookActive = false;
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void GameCameraController::OnKeyPressed(const KeyboardEvent& event) noexcept
{
	if (event.KeyCode == Key::Escape && m_bMouseLookActive)
	{
		m_bMouseLookActive = false;
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void GameCameraController::OnWindowResized() noexcept
{
	const float width = static_cast<float>(m_window.GetWidth());
	const float height = static_cast<float>(m_window.GetHeight());
	if (width > 0.0f && height > 0.0f)
	{
		m_camera.GetCameraComponent().SetAspectRatio(width / height);
	}
}

void GameCameraController::OnMouseWheel(const MouseWheelEvent& event) noexcept
{
	if (!event.IsVertical())
	{
		return;
	}

	CameraMovementSettings settings = m_camera.GetSettings();
	settings.moveSpeed += event.Delta * settings.speedStep;
	m_camera.SetSettings(settings);
}