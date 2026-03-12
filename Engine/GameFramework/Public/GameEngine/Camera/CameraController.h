// ============================================================================
// CameraController.h
// ----------------------------------------------------------------------------
// Connects InputSystem and Window to GameCamera. Handles WASD movement,
// mouse look, and aspect ratio updates.
//
// USAGE:
//   CameraController controller(timer, inputSystem, window, camera);
//   // Each frame:
//   controller.Update();
//
// DESIGN:
//   - Bridges input/window and camera (camera is pure data)
//   - Subscribes to input events for mouse capture
//   - Subscribes to window resize for aspect ratio
//   - Polls InputState for movement
//   - Uses ScopedEventHandle for RAII event cleanup
//
// CONTROLS:
//   - WASD: Move forward/left/back/right
//   - QE: Move up/down
//   - Right Mouse Button (hold): Enable mouse-look
//   - Mouse Move (while RMB held): Rotate camera
//   - Mouse Wheel: Adjust movement speed
//   - Shift (hold): Sprint
//   - Escape: Release mouse capture
//
// ============================================================================

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

// ============================================================================
// CameraMovementSettings - Configurable movement parameters
// ============================================================================

struct CameraMovementSettings
{
	float moveSpeed = 0.15f;           ///< Base movement speed (units/sec)
	float minMoveSpeed = 0.01f;        ///< Minimum speed (mouse wheel lower bound)
	float maxMoveSpeed = 10.0f;        ///< Maximum speed (mouse wheel upper bound)
	float speedStep = 0.1f;            ///< Speed change per scroll notch
	float sprintMultiplier = 2.0f;     ///< Speed multiplier when holding shift
	float mouseSensitivity = 0.0015f;  ///< Mouse look sensitivity (radians/pixel)
	bool invertY = false;              ///< Invert mouse Y axis
};

// ============================================================================
// CameraController
// ============================================================================

class SPARKLE_ENGINE_API CameraController final
{
  public:
	CameraController(Timer& timer, InputSystem& inputSystem, Window& window, GameCamera& camera) noexcept;
	~CameraController() noexcept;

	CameraController(const CameraController&) = delete;
	CameraController& operator=(const CameraController&) = delete;
	CameraController(CameraController&&) = delete;
	CameraController& operator=(CameraController&&) = delete;

	/// Updates camera based on current input. Call once per frame.
	void Update() noexcept;

	// ========================================================================
	// Settings Access
	// ========================================================================

	/// Returns mutable reference to movement settings for UI/config binding.
	CameraMovementSettings& GetSettings() noexcept { return m_settings; }
	const CameraMovementSettings& GetSettings() const noexcept { return m_settings; }

	/// Sets all movement settings at once.
	void SetSettings(const CameraMovementSettings& settings) noexcept { m_settings = settings; }

	// ========================================================================
	// Convenience Accessors (delegate to settings)
	// ========================================================================

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

	// Event subscriptions with RAII cleanup
	ScopedEventHandle m_mouseButtonPressedHandle;
	ScopedEventHandle m_mouseButtonReleasedHandle;
	ScopedEventHandle m_keyPressedHandle;
	ScopedEventHandle m_windowResizeHandle;
	ScopedEventHandle m_mouseWheelHandle;

	// Movement configuration
	CameraMovementSettings m_settings;

	bool m_bMouseLookActive = false;
};
