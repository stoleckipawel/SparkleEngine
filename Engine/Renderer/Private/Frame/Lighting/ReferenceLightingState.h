#pragma once

#include <cstdint>

class Texture;
struct FrameContext;

std::uint64_t BuildReferenceLightingSettingsKey() noexcept;

std::uint64_t BuildReferenceLightingStateKey(const FrameContext& frame, const Texture* environmentTexture) noexcept;
