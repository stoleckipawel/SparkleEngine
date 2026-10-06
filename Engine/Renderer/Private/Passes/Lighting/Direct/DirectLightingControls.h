#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"

#include <cstdint>

bool IsDirectLightingAdmitted() noexcept;
bool IsDirectDiffuseActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
bool IsDirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
bool IsDirectSubsurfaceActive(FrameGraphTextureHandle subsurfaceInput, FrameGraphTextureHandle subsurfaceOutput) noexcept;
std::uint64_t AppendDirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept;
