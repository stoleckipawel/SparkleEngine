#pragma once

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

struct RenderViewData
{
	PerViewConstantBufferData perViewData = {};
	PerTemporalConstantBufferData perTemporalData = {};
	RhiGpuVirtualAddress perViewGpuAddress = 0;
	RhiViewport viewport = {};
	RhiRect scissorRect = {};
};
