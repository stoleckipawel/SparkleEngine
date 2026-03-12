// ============================================================================
// ResourceState.h
// ----------------------------------------------------------------------------
// API-agnostic resource state enumeration for the Frame Graph system.
//
// PURPOSE:
//   Abstracts GPU resource states away from D3D12 specifics, providing a clean
//   boundary for render passes. The RenderContext translates these states to
//   the appropriate D3D12_RESOURCE_STATES when issuing barriers.
//
#pragma once

#include <cstdint>

// ============================================================================
// Resource State Enumeration
// ============================================================================

/// GPU resource state for barrier transitions.
/// Used by RenderContext to abstract D3D12 resource barriers.
enum class ResourceState : std::uint8_t
{
	Common,           ///< Initial/final state, general purpose
	RenderTarget,     ///< Writing to color buffer (render target)
	DepthWrite,       ///< Writing to depth buffer
	DepthRead,        ///< Sampling depth buffer (read-only)
	ShaderResource,   ///< Sampling in pixel shader (SRV)
	UnorderedAccess,  ///< Compute read/write (UAV)
	CopySource,       ///< Source for copy operation
	CopyDest,         ///< Destination for copy operation
	Present,          ///< Swap chain present state

	Count
};

/// Convert ResourceState to string for debugging/logging.
constexpr const char* ResourceStateToString(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
			return "Common";
		case ResourceState::RenderTarget:
			return "RenderTarget";
		case ResourceState::DepthWrite:
			return "DepthWrite";
		case ResourceState::DepthRead:
			return "DepthRead";
		case ResourceState::ShaderResource:
			return "ShaderResource";
		case ResourceState::UnorderedAccess:
			return "UnorderedAccess";
		case ResourceState::CopySource:
			return "CopySource";
		case ResourceState::CopyDest:
			return "CopyDest";
		case ResourceState::Present:
			return "Present";
		default:
			return "Unknown";
	}
}
