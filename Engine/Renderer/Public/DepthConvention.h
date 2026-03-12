// ============================================================================
// DepthConvention.h
// ----------------------------------------------------------------------------
// Centralized authority for depth buffer configuration and projection matrices.
//
// USAGE:
//   auto mode = GDepthConvention.GetMode();
//   auto clearVal = GDepthConvention.GetClearDepth();
//   auto compFunc = GDepthConvention.GetComparisonFunc();
//   // Subscribe to mode changes:
//   auto handle = DepthConvention::OnModeChanged.Add([](DepthMode m) { ... });
//
// DESIGN:
//   - Single source of truth for depth range, comparison, and clear values
//   - Event broadcast when mode changes (Camera, DepthStencil listen)
//   - Supports Standard (0-1) and ReversedZ (1-0) modes
//
// CONSISTENCY:
//   - Projection matrix generation (Camera)
//   - Depth buffer creation and clearing (D3D12DepthStencil)
//   - Pipeline state depth comparison (D3D12PipelineState)
//   - Shader depth reconstruction
//
// NOTES:
//   - ReversedZ recommended for better precision distribution
//   - Singleton accessed via GDepthConvention global reference
// ============================================================================

#pragma once

#include "Event.h"
#include <DirectXMath.h>
#include <d3d12.h>
#include <cstdint>

// ============================================================================
// Depth Mode Enumeration
// ============================================================================

/// Depth buffer mode selection.
enum class DepthMode : std::uint8_t
{
	Standard,   // Traditional depth: near=0, far=1
	ReversedZ,  // Reversed depth: near=1, far=0 (better precision)
	Count
};

// String conversion for UI display
constexpr const char* DepthModeToString(DepthMode mode) noexcept
{
	switch (mode)
	{
		case DepthMode::Standard:
			return "Standard (Near=0, Far=1)";
		case DepthMode::ReversedZ:
			return "Reversed-Z (Near=1, Far=0)";
		default:
			return "Unknown";
	}
}

//------------------------------------------------------------------------------
// DepthConvention - Global depth configuration singleton
//------------------------------------------------------------------------------
class DepthConvention
{
  public:
	// Event signature: void(DepthMode newMode)
	using OnModeChangedEvent = Event<void(DepthMode)>;

	// Event broadcast when depth mode changes. Add listener to react to changes.
	static OnModeChangedEvent OnModeChanged;

	// Configuration
	static void SetMode(DepthMode mode) noexcept;
	static DepthMode GetMode() noexcept;
	static bool IsReversedZ() noexcept;

	//--------------------------------------------------------------------------
	// Depth Buffer Configuration
	//--------------------------------------------------------------------------

	// Value to clear depth buffer to (0.0 for reversed-Z, 1.0 for standard)
	static float GetClearDepth() noexcept;

	// Depth comparison function for opaque geometry
	static D3D12_COMPARISON_FUNC GetDepthComparisonLessEqualFunc() noexcept;

	// Depth comparison function with equality (for depth-equal passes)
	static D3D12_COMPARISON_FUNC GetDepthComparisonFuncEqual() noexcept;

	//--------------------------------------------------------------------------
	// Projection Matrix Generation (Left-Handed, Z in [0,1])
	//--------------------------------------------------------------------------

	// Perspective projection with finite near/far planes.
	// Automatically applies correct depth mapping based on current mode.
	static DirectX::XMMATRIX CreatePerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) noexcept;

	//--------------------------------------------------------------------------
	// Depth Linearization (for shaders / debug visualization)
	//--------------------------------------------------------------------------

	// Convert NDC depth [0,1] to linear view-space Z
	static float LinearizeDepth(float ndcDepth, float nearZ, float farZ) noexcept;

	// Convert linear view-space Z to NDC depth [0,1]
	static float DepthToNDC(float linearZ, float nearZ, float farZ) noexcept;

  private:
	static DepthMode s_mode;

	// Static utility class - no instantiation needed
	DepthConvention() = delete;
	~DepthConvention() = delete;
};
