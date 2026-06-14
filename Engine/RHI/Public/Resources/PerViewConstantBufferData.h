#pragma once

#include "RenderConstantBufferValidation.h"
#include "RenderViewCameraData.h"
#include "RenderViewLightingData.h"

#include <cstddef>

struct alignas(256) PerViewConstantBufferData
{
	PerViewCameraConstantBufferData Camera = {};
	PerViewLightingConstantBufferData ViewLighting = {};
};
RHI_CBV_CHECK(PerViewConstantBufferData);
static_assert(offsetof(PerViewConstantBufferData, Camera) == 0, "PerViewConstantBufferData::Camera must start at c0");
static_assert(
    offsetof(PerViewConstantBufferData, ViewLighting) == 352,
    "PerViewConstantBufferData::ViewLighting must start after camera data");
static_assert(sizeof(PerViewConstantBufferData) == 57856, "PerViewConstantBufferData must fit in aligned CBV slots");
