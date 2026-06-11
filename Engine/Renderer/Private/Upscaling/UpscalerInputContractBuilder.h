#pragma once

#include "Upscaling/UpscalerInputContract.h"

struct UpscalerInputContractBuildDesc final
{
	RenderProductHandle HudlessSceneColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle FinalOutput = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
	RenderTemporalFrameState TemporalState = {};
};

UpscalerInputContract BuildUpscalerInputContract(const UpscalerInputContractBuildDesc& desc);
