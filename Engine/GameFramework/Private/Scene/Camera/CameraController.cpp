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

#include <DirectXMath.h>

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

void CameraController::SetSettings(const CameraMovementSettings& settings) noexcept
{
	m_settings = settings;
	SetMoveSpeed(settings.moveSpeed);
}

void CameraController::ApplyCameraDesc(const CameraDesc& cameraDesc) noexcept
{
	SetPosition(cameraDesc.position);
	SetYawPitch(cameraDesc.yawRadians, cameraDesc.pitchRadians);
	SetFovYDegrees(cameraDesc.fovYDegrees);
	SetMoveSpeed(cameraDesc.moveSpeed);
}

CameraDesc CameraController::CaptureCurrentCameraDesc() const noexcept
{
	return CameraDesc::FromRuntimeState(
	    GetPosition(),
	    GetYaw(),
	    GetPitch(),
	    GetFovYDegrees(),
	    GetMoveSpeed());
}

void CameraController::SetPosition(const DirectX::XMFLOAT3& position) noexcept
{
	m_camera.SetPosition(position);
}

void CameraController::SetYaw(float yawRadians) noexcept
{
	SetYawPitch(yawRadians, GetPitch());
}

void CameraController::SetPitch(float pitchRadians) noexcept
{
	SetYawPitch(GetYaw(), pitchRadians);
}

void CameraController::SetYawPitch(float yawRadians, float pitchRadians) noexcept
{
	m_camera.SetYawPitch(yawRadians, ClampPitch(pitchRadians));
}

void CameraController::SetMoveSpeed(float speed) noexcept
{
	m_settings.moveSpeed = ClampMoveSpeed(speed);
}

void CameraController::SetFovYDegrees(float fovDegrees) noexcept
{
	m_camera.SetFovYDegrees(ClampFovYDegrees(fovDegrees));
}

DirectX::XMFLOAT3 CameraController::GetPosition() const noexcept
{
	return m_camera.GetPosition();
}

float CameraController::GetYaw() const noexcept
{
	return m_camera.GetYaw();
}

float CameraController::GetPitch() const noexcept
{
	return m_camera.GetPitch();
}

float CameraController::GetFovYDegrees() const noexcept
{
	return m_camera.GetFovYDegrees();
}

float CameraController::ClampPitch(float pitchRadians) noexcept
{
	constexpr float maxPitch = DirectX::XM_PIDIV2 - 0.01f;
	return std::clamp(pitchRadians, -maxPitch, maxPitch);
}

float CameraController::ClampMoveSpeed(float speed) const noexcept
{
	return std::clamp(speed, m_settings.minMoveSpeed, m_settings.maxMoveSpeed);
}

float CameraController::ClampFovYDegrees(float fovDegrees) const noexcept
{
	return std::clamp(fovDegrees, 1.0f, 179.0f);
}

void CameraController::Update() noexcept
{
	const InputState& input = m_inputSystem.GetState();
	const float deltaTime = static_cast<float>(m_timer.GetDelta(TimeDomain::Scaled));

	if (m_bMouseLookActive)
	{
		const MousePosition mouseDelta = input.GetMouseDelta();

		const float ySign = GetInvertY() ? 1.0f : -1.0f;

		const float yawDelta = static_cast<float>(mouseDelta.X) * GetMouseSensitivity();
		const float pitchDelta = ySign * static_cast<float>(mouseDelta.Y) * GetMouseSensitivity();

		SetYawPitch(GetYaw() + yawDelta, GetPitch() + pitchDelta);

		m_inputSystem.CenterCursor(m_window.GetHWND());
	}

	if (deltaTime <= 0.0f)
	{
		return;
	}

	float speed = GetMoveSpeed();
	if (input.IsKeyDown(Key::LeftShift) || input.IsKeyDown(Key::RightShift))
	{
		speed *= GetSprintMultiplier();
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

	SetMoveSpeed(m_settings.moveSpeed + (event.Delta * m_settings.speedStep));
}
