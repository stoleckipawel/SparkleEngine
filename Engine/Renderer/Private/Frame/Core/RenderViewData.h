#pragma once

#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "ShaderData/PerTemporalConstantBufferData.h"
#include "ShaderData/PerViewConstantBufferData.h"
#include "Frame/Temporal/TemporalFrameState.h"

struct RenderViewData
{
	PerViewConstantBufferData perViewData = {};
	PerTemporalConstantBufferData perTemporalData = {};
	RenderTemporalFrameState temporalState = {};
	RhiViewport viewport = {};
	RhiRect scissorRect = {};
};
