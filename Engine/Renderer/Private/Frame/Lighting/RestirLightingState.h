#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildRestirLightingHistoryKey(const FrameContext& frame) noexcept;
