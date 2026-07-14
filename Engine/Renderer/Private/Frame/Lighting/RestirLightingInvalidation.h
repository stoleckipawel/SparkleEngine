#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildRestirLightingHistoryInvalidationHash(const FrameContext& frame) noexcept;
