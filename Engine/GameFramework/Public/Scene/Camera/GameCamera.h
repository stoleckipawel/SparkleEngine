// ============================================================================
// GameCamera.h
// ----------------------------------------------------------------------------
// Game-side camera. Pure data object with transform/projection state.
//
// USAGE:
//   GameCamera camera;
//   camera.Move(direction, distance);
//   camera.Rotate(yawDelta, pitchDelta);
//
// DESIGN:
//   - Pure data object, no knowledge of external systems
//   - External controllers (CameraController) handle input and window events
//   - Provides movement methods for controllers to use
//   - Uses dirty flag to signal when matrices need rebuilding
//   - Caches direction vector to avoid redundant trig calculations
//
// ============================================================================

#pragma once

#include <DirectXMath.h>

class GameCamera final
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	GameCamera() noexcept;
	~GameCamera() noexcept = default;

	GameCamera(const GameCamera&) = delete;
	GameCamera& operator=(const GameCamera&) = delete;
	GameCamera(GameCamera&&) = delete;
	GameCamera& operator=(GameCamera&&) = delete;

	// ========================================================================
	// Movement
	// ========================================================================

	/// Moves the camera along a direction vector by the given distance.
	void Move(const DirectX::XMFLOAT3& direction, float distance) noexcept;

	/// Moves the camera forward/backward (negative = backward).
	void MoveForward(float distance) noexcept;

	/// Moves the camera right/left (negative = left).
	void MoveRight(float distance) noexcept;

	/// Moves the camera up/down (negative = down).
	void MoveUp(float distance) noexcept;

	/// Rotates the camera by yaw and pitch deltas (radians).
	void Rotate(float yawDelta, float pitchDelta) noexcept;

	// ========================================================================
	// Transform
	// ========================================================================

	DirectX::XMFLOAT3 GetPosition() const noexcept { return m_position; }
	void SetPosition(const DirectX::XMFLOAT3& position) noexcept;

	const DirectX::XMFLOAT3& GetDirection() const noexcept;
	DirectX::XMFLOAT3 GetRight() const noexcept;

	void SetYawPitch(float yawRadians, float pitchRadians) noexcept;
	float GetYaw() const noexcept { return m_yaw; }
	float GetPitch() const noexcept { return m_pitch; }

	// ========================================================================
	// Dirty Flag (for RenderCamera optimization)
	// ========================================================================

	/// Returns true if camera state changed since last ClearDirty().
	bool IsDirty() const noexcept { return m_dirty; }

	/// Clears the dirty flag. Called by RenderCamera after rebuilding matrices.
	void ClearDirty() noexcept { m_dirty = false; }

	// ========================================================================
	// Projection
	// ========================================================================

	float GetFovYDegrees() const noexcept { return m_fovYDegrees; }
	void SetFovYDegrees(float fovDegrees) noexcept { m_fovYDegrees = fovDegrees; }

	float GetNearZ() const noexcept { return m_nearZ; }
	float GetFarZ() const noexcept { return m_farZ; }
	void SetNearFar(float nearZ, float farZ) noexcept
	{
		m_nearZ = nearZ;
		m_farZ = farZ;
	}

	void SetAspectRatio(float aspectRatio) noexcept;
	float GetAspectRatio() const noexcept { return m_aspectRatio; }

  private:
	/// Updates cached direction vector from current yaw/pitch.
	void UpdateCachedDirection() const noexcept;

	/// Marks camera as dirty (needs matrix rebuild).
	void MarkDirty() noexcept { m_dirty = true; }

	// ------------------------------------------------------------------------
	// Transform State
	// ------------------------------------------------------------------------

	DirectX::XMFLOAT3 m_position = {0.0f, 0.0f, -4.0f};
	float m_pitch = 0.0f;
	float m_yaw = 0.0f;

	// Cached direction (mutable for lazy evaluation in const getter)
	mutable DirectX::XMFLOAT3 m_cachedDirection = {0.0f, 0.0f, 1.0f};
	mutable bool m_directionDirty = true;

	// ------------------------------------------------------------------------
	// Projection State
	// ------------------------------------------------------------------------

	float m_fovYDegrees = 60.0f;
	float m_nearZ = 0.1f;
	float m_farZ = 1000.0f;
	float m_aspectRatio = 16.0f / 9.0f;

	// ------------------------------------------------------------------------
	// Dirty Flag
	// ------------------------------------------------------------------------

	bool m_dirty = true;  ///< True when matrices need rebuilding
};
