#pragma once

#include <cstdint>

struct PreparedRenderScene;

std::uint64_t BuildRestirLightingHistoryInvalidationHash(const PreparedRenderScene& scene) noexcept;
