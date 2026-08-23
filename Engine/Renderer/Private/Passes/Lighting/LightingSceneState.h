#pragma once

#include <cstdint>

struct PreparedRenderScene;

std::uint64_t BuildLightingSceneInvalidationHash(const PreparedRenderScene& scene) noexcept;
