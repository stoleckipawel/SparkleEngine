#pragma once

#include "FrameGraphTextureHandle.h"

struct FrameGraphTextureHistory final
{
	FrameGraphTextureHandle Previous = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Current = FrameGraphTextureHandle::Invalid();

	bool IsValid() const noexcept { return Previous.IsValid() && Current.IsValid(); }
};
