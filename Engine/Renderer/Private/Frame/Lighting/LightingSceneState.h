#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildLightingSceneStateKey(const FrameContext& frame) noexcept;
