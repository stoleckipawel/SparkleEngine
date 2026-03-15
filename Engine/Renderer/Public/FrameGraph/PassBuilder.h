#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/BufferHandle.h"
#include "Renderer/Public/FrameGraph/ResourceHandle.h"
#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/FrameGraph/ResourceUsage.h"

class FrameGraph;

class SPARKLE_RENDERER_API PassBuilder final
{
  public:
	explicit PassBuilder(FrameGraph& frameGraph) noexcept;
	~PassBuilder() noexcept = default;

	PassBuilder(const PassBuilder&) = delete;
	PassBuilder& operator=(const PassBuilder&) = delete;
	PassBuilder(PassBuilder&&) = delete;
	PassBuilder& operator=(PassBuilder&&) = delete;

	ResourceHandle Read(ResourceHandle handle, ResourceUsage usage) noexcept;
	ResourceHandle Write(ResourceHandle handle, ResourceUsage usage) noexcept;
	TextureHandle Read(TextureHandle handle, ResourceUsage usage) noexcept;
	TextureHandle Write(TextureHandle handle, ResourceUsage usage) noexcept;
	BufferHandle Read(BufferHandle handle, ResourceUsage usage) noexcept;
	BufferHandle Write(BufferHandle handle, ResourceUsage usage) noexcept;

  private:
	FrameGraph* m_frameGraph = nullptr;
};