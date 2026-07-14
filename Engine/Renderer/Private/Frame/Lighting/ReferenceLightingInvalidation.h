#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildReferenceLightingHistoryInvalidationHash(const FrameContext& frame) noexcept;
