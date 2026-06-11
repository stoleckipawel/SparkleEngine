#pragma once

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
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
