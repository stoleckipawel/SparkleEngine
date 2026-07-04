#pragma once

#include "RenderConstantBufferValidation.h"
#include "RenderViewCameraData.h"

#include <cstddef>

struct alignas(256) PerViewConstantBufferData
{
	PerViewCameraConstantBufferData Camera = {};
};
RHI_CBV_CHECK(PerViewConstantBufferData);
static_assert(offsetof(PerViewConstantBufferData, Camera) == 0, "PerViewConstantBufferData::Camera must start at c0");
static_assert(sizeof(PerViewConstantBufferData) == 512, "PerViewConstantBufferData must fit in aligned CBV slots");
