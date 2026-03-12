// ============================================================================
// EventHandle.h
// ----------------------------------------------------------------------------
// Opaque handle for event subscription management.
//
#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>

// ============================================================================
// EventHandle
// ============================================================================

/// Opaque handle returned by Event::Add(), used for Event::Remove().
/// A handle with Id == 0 is considered invalid.
struct SPARKLE_CORE_API EventHandle
{
	std::uint32_t Id = 0;  ///< Unique identifier (0 = invalid)

	/// Returns true if handle points to a valid subscription.
	bool IsValid() const noexcept { return Id != 0; }

	/// Marks this handle as invalid.
	void Invalidate() noexcept { Id = 0; }

	bool operator==(const EventHandle& Other) const noexcept { return Id == Other.Id; }
	bool operator!=(const EventHandle& Other) const noexcept { return Id != Other.Id; }
};
