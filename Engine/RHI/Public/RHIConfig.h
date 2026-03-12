// ============================================================================
// RHIConfig.h
// ----------------------------------------------------------------------------
// RHI module configuration constants and compile-time toggles.
//
#pragma once

#include <dxgi1_6.h>

// ============================================================================
// Compile-Time Feature Toggles
// ============================================================================

/// Shader compilation flags (enabled by default in debug builds).
#if defined(_DEBUG)
	#define ENGINE_SHADERS_OPTIMIZED 1  ///< Enable shader optimizations
	#define ENGINE_SHADERS_DEBUG 1      ///< Include shader debug info
#endif

/// GPU validation layers (D3D12/DXGI SDK layers).
#if defined(_DEBUG)
	#define ENGINE_GPU_VALIDATION 1
#endif

/// Report live D3D/DXGI objects at shutdown for leak detection.
#if defined(_DEBUG)
	#define ENGINE_REPORT_LIVE_OBJECTS 1
#endif

// ============================================================================
// Runtime Configuration
// ============================================================================

namespace RHISettings
{
	// ------------------------------------------------------------------------
	// Rendering
	// ------------------------------------------------------------------------

	/// Number of frames that can be processed simultaneously.
	/// Higher values reduce CPU-GPU sync but increase latency and memory.
	inline constexpr unsigned FramesInFlight = 2u;

	/// Back buffer pixel format.
	inline constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	/// Depth stencil buffer format.
	inline constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	/// Enable vertical sync. False allows uncapped presents or tearing.
	inline bool VSync = true;

	// ------------------------------------------------------------------------
	// Hardware
	// ------------------------------------------------------------------------

	/// Prefer high-performance GPU when enumerating adapters.
	inline bool PreferHighPerformanceAdapter = true;

	// ------------------------------------------------------------------------
	// Shaders
	// ------------------------------------------------------------------------

	/// Target shader model version (e.g., 6.0 for SM 6.0).
	inline constexpr int ShaderModelMajor = 6;
	inline constexpr int ShaderModelMinor = 0;

}  // namespace RHISettings
