#pragma once

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/PerTemporalConstantBufferData.h"
#include "RHI/Public/Resources/PerViewConstantBufferData.h"
#include "Frame/Temporal/TemporalFrameState.h"

struct RenderViewData
{
	PerViewConstantBufferData perViewData = {};
	PerTemporalConstantBufferData perTemporalData = {};
	RenderTemporalFrameState temporalState = {};
	RhiViewport viewport = {};
	RhiRect scissorRect = {};
};
