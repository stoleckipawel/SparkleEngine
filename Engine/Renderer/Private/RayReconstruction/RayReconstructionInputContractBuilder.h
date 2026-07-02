#pragma once

#include "RayReconstruction/RayReconstructionInputContract.h"

struct RayReconstructionInputContractBuildDesc final
{
	RenderProductHandle NoisyInputColor = {};
	RenderProductHandle OutputColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle Exposure = {};
	RenderProductHandle Normals = {};
	RenderProductHandle Roughness = {};
	RenderProductHandle DiffuseAlbedo = {};
	RenderProductHandle SpecularAlbedo = {};
	RenderProductHandle SpecularHitDistance = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
};

RayReconstructionInputContract BuildRayReconstructionInputContract(const RayReconstructionInputContractBuildDesc& desc);
