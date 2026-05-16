#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

#include <cstdint>
#include <string>

enum class FrameGraphResourceClass : std::uint8_t
{
	Texture,
	Buffer
};

enum class FrameGraphResourceKind : std::uint8_t
{
	BackBuffer,
	DepthStencil,
	ColorRenderTarget,
	Buffer
};

enum class FrameGraphResourceOwnership : std::uint8_t
{
	Transient,
	Imported,
};

struct FrameGraphResourceMetadata
{
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
	FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
	FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
	ResourceState initialState = ResourceState::Common;
	ResourceState finalState = ResourceState::Common;
	std::string debugName;
	FrameGraphTextureDesc textureDesc{};
	FrameGraphBufferDesc bufferDesc{};
};
