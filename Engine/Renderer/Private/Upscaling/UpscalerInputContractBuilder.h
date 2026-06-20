#pragma once

#include "Upscaling/UpscalerInputContract.h"

struct UpscalerInputContractBuildDesc final
{
	RenderProductHandle HudlessSceneColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle Normals = {};
	RenderProductHandle FinalOutput = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
};

UpscalerInputContract BuildUpscalerInputContract(const UpscalerInputContractBuildDesc& desc);
