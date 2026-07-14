#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildReferenceLightingHistoryKey(const FrameContext& frame) noexcept;
