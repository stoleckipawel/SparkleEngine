#pragma once

#include "Upscaling/UpscalerInputContract.h"

struct UpscalerInputContractBuildDesc final
{
	RenderProductHandle ScalingInputColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle Exposure = {};
	RenderProductHandle ScalingOutputColor = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
	bool ExposureRequired = false;
};

UpscalerInputContract BuildUpscalerInputContract(const UpscalerInputContractBuildDesc& desc);
