#pragma once

#include <cstdint>

struct FrameContext;

std::uint64_t BuildReferenceLightingSettingsKey() noexcept;

std::uint64_t BuildReferenceLightingStateKey(const FrameContext& frame) noexcept;
