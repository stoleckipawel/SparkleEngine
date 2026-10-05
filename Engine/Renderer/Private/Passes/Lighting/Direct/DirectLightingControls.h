#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Passes/Lighting/Direct/DirectLightingUniformData.h"

#include <cstdint>

bool IsDirectDiffuseEnabled() noexcept;
bool IsDirectSpecularEnabled() noexcept;
bool IsDirectSubsurfaceEnabled() noexcept;
bool IsDirectLightingAdmitted() noexcept;
bool IsDirectDiffuseActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
bool IsDirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
bool IsDirectSubsurfaceActive(FrameGraphTextureHandle subsurfaceInput, FrameGraphTextureHandle subsurfaceOutput) noexcept;
DirectLightingUniformData BuildDirectLightingUniform(
    FrameGraphTextureHandle baseColorInput,
    FrameGraphTextureHandle materialInput,
    FrameGraphTextureHandle subsurfaceInput,
    FrameGraphTextureHandle diffuseOutput,
    FrameGraphTextureHandle specularOutput,
    FrameGraphTextureHandle subsurfaceOutput) noexcept;
std::uint64_t AppendDirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept;
