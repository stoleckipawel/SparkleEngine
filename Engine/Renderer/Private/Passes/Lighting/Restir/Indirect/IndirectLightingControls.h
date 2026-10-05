#pragma once
#include "FrameGraph/FrameGraphTextureHandle.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include <cstdint>

bool IsIndirectDiffuseEnabled() noexcept;
bool IsIndirectSpecularEnabled() noexcept;
bool IsIndirectLightingAdmitted() noexcept;
bool IsIndirectShadowsEnabled() noexcept;
bool IsIndirectShadowsActive() noexcept;
bool IsIndirectDiffuseActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
bool IsIndirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
RestirIndirectLightingUniformData BuildIndirectLightingUniform(
    FrameGraphTextureHandle baseColorInput,
    FrameGraphTextureHandle materialInput,
    FrameGraphTextureHandle diffuseOutput,
    FrameGraphTextureHandle specularOutput) noexcept;
std::uint64_t AppendIndirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept;
