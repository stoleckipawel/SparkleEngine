#pragma once
#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Core/Public/Console/CVar.h"
#include <cstdint>

extern ConsoleVariable<bool> CVarIndirectDiffuse;
extern ConsoleVariable<bool> CVarIndirectSpecular;
bool IsIndirectLightingAdmitted() noexcept;
extern ConsoleVariable<bool> CVarIndirectShadows;
bool IsIndirectShadowsActive() noexcept;
bool IsIndirectDiffuseActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
bool IsIndirectSpecularActive(FrameGraphTextureHandle input, FrameGraphTextureHandle output) noexcept;
std::uint64_t AppendIndirectLightingHistoryInvalidationHash(std::uint64_t hash) noexcept;
