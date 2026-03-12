// ============================================================================
// ResourceHandle.h
// ----------------------------------------------------------------------------
// Opaque handle to a Frame Graph resource (texture, buffer, etc.).
//
// PURPOSE:
//   Identifies resources within the Frame Graph without exposing GPU details.
//   Passes use handles to declare resource dependencies during Setup phase.
//
// USAGE:
//   ResourceHandle backBuffer = builder.UseBackBuffer();
//   ResourceHandle depth = builder.UseDepthBuffer();
//   if (handle.IsValid()) { ... }
//
// DESIGN:
//   - Simple value type (copyable, comparable)
//   - Well-known handles for MVP: BACKBUFFER, DEPTH_BUFFER
//   - INVALID handle for error/uninitialized state
//   - Future: dynamic handle allocation for transient resources
//
// NOTES:
//   - No GPU types — pure index into Frame Graph resource registry
//   - Equality comparable for use in containers
// ============================================================================

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

	/// Creates an invalid handle.
	static constexpr ResourceHandle Invalid() noexcept { return ResourceHandle{INVALID_INDEX}; }

	/// Creates a back buffer handle.
	static constexpr ResourceHandle BackBuffer() noexcept { return ResourceHandle{BACKBUFFER_INDEX}; }

	/// Creates a depth buffer handle.
	static constexpr ResourceHandle DepthBuffer() noexcept { return ResourceHandle{DEPTH_BUFFER_INDEX}; }

	// -------------------------------------------------------------------------
	// Validation
	// -------------------------------------------------------------------------

	/// Returns true if this handle refers to a valid resource.
	constexpr bool IsValid() const noexcept { return index != INVALID_INDEX; }

	/// Returns true if this is the back buffer handle.
	constexpr bool IsBackBuffer() const noexcept { return index == BACKBUFFER_INDEX; }

	/// Returns true if this is the depth buffer handle.
	constexpr bool IsDepthBuffer() const noexcept { return index == DEPTH_BUFFER_INDEX; }

	// -------------------------------------------------------------------------
	// Comparison (C++20 three-way)
	// -------------------------------------------------------------------------

	constexpr auto operator<=>(const ResourceHandle&) const noexcept = default;
};
