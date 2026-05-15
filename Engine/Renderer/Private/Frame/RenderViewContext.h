#pragma once

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

struct RenderViewContext
{
	PerViewConstantBufferData perViewData = {};
	RhiGpuVirtualAddress perViewGpuAddress = 0;
	RhiViewport viewport = {};
	RhiRect scissorRect = {};
};
