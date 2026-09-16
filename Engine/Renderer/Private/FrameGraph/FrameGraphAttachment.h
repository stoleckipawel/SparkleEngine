#pragma once

#include "FrameGraphTextureHandle.h"

#include <cstdint>

enum class FrameGraphAttachmentLoadAction : std::uint8_t
{
	Load = 0,
	Clear = 1,
};

enum class FrameGraphAttachmentStoreAction : std::uint8_t
{
	Store = 0,
	Discard = 1,
};

enum class FrameGraphDepthStencilAccess : std::uint8_t
{
	ReadOnly = 0,
	ReadWrite = 1,
};

struct FrameGraphAttachmentBinding final
{
	FrameGraphTextureHandle Handle = FrameGraphTextureHandle::Invalid();
	FrameGraphAttachmentLoadAction Load = FrameGraphAttachmentLoadAction::Load;
	FrameGraphAttachmentStoreAction Store = FrameGraphAttachmentStoreAction::Store;
	FrameGraphDepthStencilAccess DepthStencilAccess = FrameGraphDepthStencilAccess::ReadWrite;
};
