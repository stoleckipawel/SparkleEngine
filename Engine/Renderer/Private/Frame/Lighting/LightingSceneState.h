#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildLightingSceneInvalidationHash(const FrameContext& frame) noexcept;
