// =============================================================================
// PassBuilder.h - Resource Declaration Interface for Render Passes
// =============================================================================
//
// Builder interface passed to RenderPass::Setup() for declaring resource usage.
// Passes use PassBuilder to state which resources they read from and write to.
// The FrameGraph uses this to manage lifetimes and barrier transitions.
//
#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/ResourceHandle.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"

// =============================================================================
// PassBuilder
// =============================================================================

class SPARKLE_RENDERER_API PassBuilder
{
  public:
	PassBuilder() = default;
	~PassBuilder() = default;

	PassBuilder(const PassBuilder&) = delete;
	PassBuilder& operator=(const PassBuilder&) = delete;
	PassBuilder(PassBuilder&&) = delete;
	PassBuilder& operator=(PassBuilder&&) = delete;

	// -------------------------------------------------------------------------
	// Well-Known Resources
	// -------------------------------------------------------------------------

	/// Declares write access to the swap chain back buffer.
	ResourceHandle UseBackBuffer() noexcept { return ResourceHandle::BackBuffer(); }

	/// Declares write access to the depth buffer.
	ResourceHandle UseDepthBuffer() noexcept { return ResourceHandle::DepthBuffer(); }

	// -------------------------------------------------------------------------
	// Future API (stubbed for MVP)
	// -------------------------------------------------------------------------

	/// Declares read access to a resource.
	ResourceHandle Read([[maybe_unused]] ResourceHandle handle, [[maybe_unused]] ResourceState state) noexcept
	{
		return handle;
	}

	/// Declares write access to a resource.
	ResourceHandle Write([[maybe_unused]] ResourceHandle handle, [[maybe_unused]] ResourceState state) noexcept
	{
		return handle;
	}

	/// Creates a transient texture resource.
	template <typename TextureDesc> ResourceHandle CreateTexture([[maybe_unused]] const TextureDesc& desc) noexcept
	{
		return ResourceHandle::Invalid();
	}
};
