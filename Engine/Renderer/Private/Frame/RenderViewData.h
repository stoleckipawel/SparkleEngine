#pragma once

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/PerTemporalConstantBufferData.h"
#include "RHI/Public/Resources/PerViewConstantBufferData.h"
#include "Frame/TemporalFrameState.h"

struct RenderViewData
{
	PerViewConstantBufferData perViewData = {};
	PerTemporalConstantBufferData perTemporalData = {};
	RenderTemporalFrameState temporalState = {};
	RhiGpuVirtualAddress perViewGpuAddress = 0;
	RhiViewport viewport = {};
	RhiRect scissorRect = {};
};
