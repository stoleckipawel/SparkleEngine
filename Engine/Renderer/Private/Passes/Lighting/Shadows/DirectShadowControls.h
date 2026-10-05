#pragma once

#include <cstdint>

bool IsDirectShadowsEnabled() noexcept;
bool IsDirectShadowsActive() noexcept;
void RequireDirectShadowSignal(bool available) noexcept;
std::uint64_t AppendDirectShadowHistoryInvalidationHash(std::uint64_t hash) noexcept;
