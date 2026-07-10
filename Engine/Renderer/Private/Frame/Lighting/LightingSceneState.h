#pragma once

#include <cstdint>

class Texture;
struct FrameContext;

std::uint64_t BuildLightingSceneStateKey(const FrameContext& frame, const Texture* environmentTexture) noexcept;
