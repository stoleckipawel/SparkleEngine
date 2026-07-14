#pragma once

#include "RenderViewCameraData.h"

#include <cstddef>
#include <type_traits>

struct alignas(256) PerViewConstantBufferData
{
	PerViewCameraConstantBufferData Camera = {};
};
static_assert(std::is_standard_layout_v<PerViewConstantBufferData>);
static_assert(std::is_trivially_copyable_v<PerViewConstantBufferData>);
static_assert(alignof(PerViewConstantBufferData) >= 256);
static_assert(sizeof(PerViewConstantBufferData) % 256 == 0);
static_assert(sizeof(PerViewConstantBufferData) <= 64 * 1024);
static_assert(offsetof(PerViewConstantBufferData, Camera) == 0, "PerViewConstantBufferData::Camera must start at c0");
static_assert(sizeof(PerViewConstantBufferData) == 512, "PerViewConstantBufferData must fit in aligned CBV slots");
