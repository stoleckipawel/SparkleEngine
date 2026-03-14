#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/ResourceHandle.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"

class SPARKLE_RENDERER_API PassBuilder
{
  public:
	PassBuilder() = default;
	~PassBuilder() = default;

	PassBuilder(const PassBuilder&) = delete;
	PassBuilder& operator=(const PassBuilder&) = delete;
	PassBuilder(PassBuilder&&) = delete;
	PassBuilder& operator=(PassBuilder&&) = delete;

	ResourceHandle UseBackBuffer() noexcept { return ResourceHandle::BackBuffer(); }

	ResourceHandle UseDepthBuffer() noexcept { return ResourceHandle::DepthBuffer(); }

	ResourceHandle Read([[maybe_unused]] ResourceHandle handle, [[maybe_unused]] ResourceState state) noexcept { return handle; }

	ResourceHandle Write([[maybe_unused]] ResourceHandle handle, [[maybe_unused]] ResourceState state) noexcept { return handle; }

	template <typename TextureDesc> ResourceHandle CreateTexture([[maybe_unused]] const TextureDesc& desc) noexcept
	{
		return ResourceHandle::Invalid();
	}
};
