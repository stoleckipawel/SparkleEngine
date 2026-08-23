#pragma once

#include <cstdint>

struct PreparedRenderScene;
struct RenderView;

std::uint64_t BuildReferenceLightingHistoryInvalidationHash(const PreparedRenderScene& scene, const RenderView& view) noexcept;
