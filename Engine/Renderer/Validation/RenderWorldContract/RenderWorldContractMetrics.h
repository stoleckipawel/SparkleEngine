#pragma once

#include "GameFramework/Public/Rendering/RenderInputFrame.h"

#include <cstddef>
#include <span>

std::size_t MeasureRenderInputOwnedBytes(const RenderInputFrame& frame) noexcept;
std::size_t MeasureRenderInputRecordingOwnedBytes(std::span<const RenderInputFrame> recording) noexcept;
