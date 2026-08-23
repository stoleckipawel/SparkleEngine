#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"

#include <cstdint>
#include <string>

enum class FrameGraphResourceClass : std::uint8_t
{
	Texture,
	Buffer,
	AccelerationStructure
};

enum class FrameGraphResourceKind : std::uint8_t
{
	BackBuffer,
	DepthStencil,
	ColorRenderTarget,
	Buffer,
	AccelerationStructure
};

enum class FrameGraphResourceOwnership : std::uint8_t
{
	Transient,
	Imported,
	ExternalPersistent,
};

constexpr bool IsExternalFrameGraphResource(FrameGraphResourceOwnership ownership) noexcept
{
	return ownership == FrameGraphResourceOwnership::Imported || ownership == FrameGraphResourceOwnership::ExternalPersistent;
}

struct FrameGraphResourceMetadata
{
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
	FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
	FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
	ResourceState initialState = ResourceState::Common;
	ResourceState finalState = ResourceState::Common;
	bool hasExternalContents = false;
	std::string debugName;
	FrameGraphTextureDesc textureDesc{};
	FrameGraphBufferDesc bufferDesc{};
};
