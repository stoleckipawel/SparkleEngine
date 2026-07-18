#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/World/GameWorldController.h"
#include "GameFramework/Public/World/EntityId.h"

#include "Events/EventHandle.h"
#include "Events/ScopedEventHandle.h"

class Timer;
class InputSystem;
class Window;
class SceneCameraView;
class GameWorld;
struct KeyboardEvent;
struct MouseButtonEvent;
struct MouseMoveEvent;
struct MouseWheelEvent;

class SPARKLE_ENGINE_API GameCameraController final : public GameWorldController
{
  public:
	GameCameraController(Timer& timer, InputSystem& inputSystem, Window& window) noexcept;
	~GameCameraController() noexcept;

	GameCameraController(const GameCameraController&) = delete;
	GameCameraController& operator=(const GameCameraController&) = delete;
	GameCameraController(GameCameraController&&) = delete;
	GameCameraController& operator=(GameCameraController&&) = delete;

	void OnWorldReset(GameWorld& world) override;
	void OnLevelLoaded(GameWorld& world, const LevelDesc& levelDesc) override;
	void OnSceneAssetsAppended(GameWorld& world) override;
	void Update(GameWorld& world, const GameWorldUpdateContext& context) override;

  private:
	void RefreshActiveCamera(GameWorld& world) noexcept;
	SceneCameraView ResolveCamera() const noexcept;
	void ApplyAspectRatio() noexcept;
	void ApplyMovement(float deltaTime) noexcept;
	void OnMouseButtonPressed(const MouseButtonEvent& event) noexcept;
	void OnMouseButtonReleased(const MouseButtonEvent& event) noexcept;
	void OnKeyboardEvent(const KeyboardEvent& event) noexcept;
	void OnMouseMove(const MouseMoveEvent& event) noexcept;
	void OnWindowResized() noexcept;
	void OnMouseWheel(const MouseWheelEvent& event) noexcept;
	void ResetMovementIntent() noexcept;

	Timer& m_timer;
	InputSystem& m_inputSystem;
	Window& m_window;
	GameWorld* m_world = nullptr;
	EntityId m_cameraEntity;

	ScopedEventHandle m_windowResizeHandle;
	EventHandle m_mouseButtonPressedHandle;
	EventHandle m_mouseButtonReleasedHandle;
	EventHandle m_mouseMoveHandle;
	EventHandle m_keyboardHandle;
	EventHandle m_mouseWheelHandle;
	bool m_forwardHeld = false;
	bool m_backwardHeld = false;
	bool m_rightHeld = false;
	bool m_leftHeld = false;
	bool m_upHeld = false;
	bool m_downHeld = false;
	bool m_sprintHeld = false;
	bool m_bMouseLookActive = false;
};
