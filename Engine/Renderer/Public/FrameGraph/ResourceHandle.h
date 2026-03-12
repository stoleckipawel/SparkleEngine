// ============================================================================
// ResourceHandle.h
// ----------------------------------------------------------------------------
// Opaque handle to a Frame Graph resource (texture, buffer, etc.).
//
// PURPOSE:
//   Identifies resources within the Frame Graph without exposing GPU details.
//   Passes use handles to declare resource dependencies during Setup phase.
//
#pragma once

#include <compare>
#include <cstdint>
#include <limits>

// ============================================================================
// ResourceHandle
// ============================================================================

/// Opaque handle to a Frame Graph resource.
/// Used by passes to declare resource dependencies without GPU coupling.
struct ResourceHandle
{
	std::uint32_t index = std::numeric_limits<std::uint32_t>::max();

	// -------------------------------------------------------------------------
	// Well-Known Handles (MVP)
	// -------------------------------------------------------------------------

	/// Invalid/uninitialized handle.
	static constexpr std::uint32_t INVALID_INDEX = std::numeric_limits<std::uint32_t>::max();

	/// Swap chain back buffer (index 0).
	static constexpr std::uint32_t BACKBUFFER_INDEX = 0;

	/// Depth-stencil buffer (index 1).
	static constexpr std::uint32_t DEPTH_BUFFER_INDEX = 1;

	// -------------------------------------------------------------------------
	// Factory Methods
	// -------------------------------------------------------------------------

	static constexpr ResourceHandle Invalid() noexcept { return ResourceHandle{INVALID_INDEX}; }

	static constexpr ResourceHandle BackBuffer() noexcept { return ResourceHandle{BACKBUFFER_INDEX}; }

	static constexpr ResourceHandle DepthBuffer() noexcept { return ResourceHandle{DEPTH_BUFFER_INDEX}; }

	// -------------------------------------------------------------------------
	// Validation
	// -------------------------------------------------------------------------

	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	constexpr bool IsBackBuffer() const noexcept { return index == BACKBUFFER_INDEX; }

	constexpr bool IsDepthBuffer() const noexcept { return index == DEPTH_BUFFER_INDEX; }

	// -------------------------------------------------------------------------
	// Comparison (C++20 three-way)
	// -------------------------------------------------------------------------

	constexpr auto operator<=>(const ResourceHandle&) const noexcept = default;
};
