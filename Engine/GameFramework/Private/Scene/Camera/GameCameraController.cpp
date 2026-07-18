#include "PCH.h"
#include "Scene/Camera/GameCameraController.h"
#include "Input/InputSystem.h"
#include "World/GameWorld.h"
#include "Scene/Camera/SceneCameraView.h"
#include "Window/Window.h"
#include "Events/ScopedEventHandle.h"
#include "Input/Keyboard/Key.h"
#include "Input/Mouse/MouseButton.h"
#include "Input/Mouse/MousePosition.h"
#include "Input/Dispatch/InputLayer.h"
#include "Input/Events/MouseMoveEvent.h"
#include "Input/Events/MouseWheelEvent.h"
#include "Core/Public/Time/Timer.h"

#include <DirectXMath.h>

#include <cmath>

GameCameraController::GameCameraController(Timer& timer, InputSystem& inputSystem, Window& window) noexcept :
    m_timer(timer), m_inputSystem(inputSystem), m_window(window)
{
	auto resizeHandle = m_window.OnResized.Add(
	    [this]()
	    {
		    OnWindowResized();
	    });
	m_windowResizeHandle = ScopedEventHandle(m_window.OnResized, resizeHandle);

	m_mouseButtonPressedHandle = m_inputSystem.SubscribeMouseButton(
	    [this](const MouseButtonEvent& event)
	    {
		    if (event.IsPressed())
		    {
			    OnMouseButtonPressed(event);
		    }
	    },
	    InputLayer::Gameplay);

	m_mouseButtonReleasedHandle = m_inputSystem.SubscribeMouseButton(
	    [this](const MouseButtonEvent& event)
	    {
		    if (event.IsReleased())
		    {
			    OnMouseButtonReleased(event);
		    }
	    },
	    InputLayer::Gameplay);

	m_mouseMoveHandle = m_inputSystem.SubscribeMouseMove(
	    [this](const MouseMoveEvent& event)
	    {
		    OnMouseMove(event);
	    },
	    InputLayer::Gameplay);

	m_keyboardHandle = m_inputSystem.SubscribeKeyboard(
	    [this](const KeyboardEvent& event)
	    {
		    OnKeyboardEvent(event);
	    },
	    InputLayer::Gameplay);

	m_mouseWheelHandle = m_inputSystem.SubscribeMouseWheel(
	    [this](const MouseWheelEvent& event)
	    {
		    OnMouseWheel(event);
	    },
	    InputLayer::Gameplay);
}

GameCameraController::~GameCameraController() noexcept
{
	m_inputSystem.Unsubscribe(m_mouseButtonPressedHandle);
	m_inputSystem.Unsubscribe(m_mouseButtonReleasedHandle);
	m_inputSystem.Unsubscribe(m_mouseMoveHandle);
	m_inputSystem.Unsubscribe(m_keyboardHandle);
	m_inputSystem.Unsubscribe(m_mouseWheelHandle);

	if (m_bMouseLookActive)
	{
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void GameCameraController::OnWorldReset(GameWorld& world)
{
	m_world = nullptr;
	m_cameraEntity = {};
	ResetMovementIntent();
}

void GameCameraController::OnLevelLoaded(GameWorld& world, const LevelDesc& levelDesc)
{
	RefreshActiveCamera(world);
}

void GameCameraController::OnSceneAssetsAppended(GameWorld& world)
{
	RefreshActiveCamera(world);
}

void GameCameraController::Update(GameWorld& world, const GameWorldUpdateContext& context)
{
	if (context.phase != GameWorldUpdatePhase::PreAnimation)
	{
		return;
	}

	RefreshActiveCamera(world);
	const float deltaTime = static_cast<float>(m_timer.GetDelta(TimeDomain::Scaled));
	if (deltaTime <= 0.0f)
	{
		return;
	}
	ApplyMovement(deltaTime);
}

void GameCameraController::ApplyMovement(float deltaTime) noexcept
{
	SceneCameraView camera = ResolveCamera();
	if (!camera.IsValid())
	{
		return;
	}

	const CameraMovementSettings settings = camera.GetMovementSettings();
	const float speed = settings.moveSpeed * (m_sprintHeld ? settings.sprintMultiplier : 1.0f);
	const float distance = speed * deltaTime;
	const float forwardAxis = (m_forwardHeld ? 1.0f : 0.0f) - (m_backwardHeld ? 1.0f : 0.0f);
	const float rightAxis = (m_rightHeld ? 1.0f : 0.0f) - (m_leftHeld ? 1.0f : 0.0f);
	const float upAxis = (m_upHeld ? 1.0f : 0.0f) - (m_downHeld ? 1.0f : 0.0f);
	if (forwardAxis == 0.0f && rightAxis == 0.0f && upAxis == 0.0f)
	{
		return;
	}

	Transform transform = camera.GetTransform();
	const DirectX::XMFLOAT3 rotation = transform.GetRotationEuler();
	const float sinYaw = std::sin(rotation.y);
	const float cosYaw = std::cos(rotation.y);
	const float cosPitch = std::cos(rotation.x);
	const float sinPitch = std::sin(rotation.x);
	transform.TranslateScaled({sinYaw * cosPitch, -sinPitch, cosYaw * cosPitch}, distance * forwardAxis);
	transform.TranslateScaled({cosYaw, 0.0f, -sinYaw}, distance * rightAxis);
	transform.TranslateScaled({0.0f, 1.0f, 0.0f}, distance * upAxis);
	camera.SetTransform(transform);
}

void GameCameraController::RefreshActiveCamera(GameWorld& world) noexcept
{
	const EntityId activeCamera = world.GetCameras().GetActiveCamera().GetEntity();
	if (m_world == &world && m_cameraEntity == activeCamera)
	{
		return;
	}
	m_world = &world;
	m_cameraEntity = activeCamera;
	ApplyAspectRatio();
}

SceneCameraView GameCameraController::ResolveCamera() const noexcept
{
	return m_world != nullptr ? m_world->GetCameras().GetCamera(m_cameraEntity) : SceneCameraView{};
}

void GameCameraController::ApplyAspectRatio() noexcept
{
	SceneCameraView camera = ResolveCamera();
	if (!camera.IsValid())
	{
		return;
	}

	const float width = static_cast<float>(m_window.GetWidth());
	const float height = static_cast<float>(m_window.GetHeight());
	if (width > 0.0f && height > 0.0f)
	{
		camera.SetAspectRatio(width / height);
	}
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
		ResetMovementIntent();
		m_inputSystem.ReleaseMouse();
		m_inputSystem.SetCursorVisibility(true);
	}
}

void GameCameraController::OnKeyboardEvent(const KeyboardEvent& event) noexcept
{
	const bool pressed = event.IsPressed();
	switch (event.KeyCode)
	{
		case Key::W:
			m_forwardHeld = pressed;
			break;
		case Key::S:
			m_backwardHeld = pressed;
			break;
		case Key::D:
			m_rightHeld = pressed;
			break;
		case Key::A:
			m_leftHeld = pressed;
			break;
		case Key::E:
		case Key::Space:
			m_upHeld = pressed;
			break;
		case Key::Q:
		case Key::C:
			m_downHeld = pressed;
			break;
		case Key::LeftShift:
		case Key::RightShift:
			m_sprintHeld = event.IsPressed();
			break;
		case Key::Escape:
			if (event.IsPressed() && m_bMouseLookActive)
			{
				m_bMouseLookActive = false;
				ResetMovementIntent();
				m_inputSystem.ReleaseMouse();
				m_inputSystem.SetCursorVisibility(true);
			}
			break;
		default:
			break;
	}
}

void GameCameraController::OnMouseMove(const MouseMoveEvent& event) noexcept
{
	if (!m_bMouseLookActive)
	{
		return;
	}
	SceneCameraView camera = ResolveCamera();
	if (!camera.IsValid())
	{
		return;
	}

	const CameraMovementSettings settings = camera.GetMovementSettings();
	Transform transform = camera.GetTransform();
	const DirectX::XMFLOAT3 rotationEuler = transform.GetRotationEuler();
	const float ySign = settings.invertY ? 1.0f : -1.0f;
	const float yawDelta = static_cast<float>(event.Delta.X) * settings.mouseSensitivity;
	const float pitchDelta = ySign * static_cast<float>(event.Delta.Y) * settings.mouseSensitivity;
	constexpr float MaxPitch = DirectX::XM_PIDIV2 - 0.01f;
	transform.SetYawPitch(rotationEuler.y + yawDelta, rotationEuler.x + pitchDelta, -MaxPitch, MaxPitch);
	camera.SetTransform(transform);
	m_inputSystem.CenterCursor(m_window.GetHWND());
}

void GameCameraController::OnWindowResized() noexcept
{
	ApplyAspectRatio();
}

void GameCameraController::OnMouseWheel(const MouseWheelEvent& event) noexcept
{
	if (!event.IsVertical())
	{
		return;
	}
	SceneCameraView camera = ResolveCamera();
	if (!camera.IsValid())
	{
		return;
	}

	CameraMovementSettings settings = camera.GetMovementSettings();
	settings.moveSpeed += event.Delta * settings.speedStep;
	camera.SetMovementSettings(settings);
}

void GameCameraController::ResetMovementIntent() noexcept
{
	m_forwardHeld = false;
	m_backwardHeld = false;
	m_rightHeld = false;
	m_leftHeld = false;
	m_upHeld = false;
	m_downHeld = false;
	m_sprintHeld = false;
}
