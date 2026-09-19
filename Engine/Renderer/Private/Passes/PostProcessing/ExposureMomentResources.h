#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"

#include <cstdint>

class FrameGraphBuilder;

struct ExposureMomentTexture final
{
	FrameGraphTextureHandle Handle = FrameGraphTextureHandle::Invalid();
	std::uint32_t Width = 1u;
	std::uint32_t Height = 1u;
};

ExposureMomentTexture CreateExposureMomentTexture(
    FrameGraphBuilder& builder,
    const char* prefix,
    std::uint32_t level,
    std::uint32_t width,
    std::uint32_t height);
