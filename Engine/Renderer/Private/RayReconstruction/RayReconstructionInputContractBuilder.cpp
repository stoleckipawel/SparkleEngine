#include "../PCH.h"
#include "RayReconstruction/RayReconstructionInputContractBuilder.h"

RayReconstructionInputContract BuildRayReconstructionInputContract(const RayReconstructionInputContractBuildDesc& desc)
{
	const bool historyInvalid = !desc.TemporalState.HistoryValid;
	return RayReconstructionInputContract{
	    .NoisyInputColor = desc.NoisyInputColor,
	    .OutputColor = desc.OutputColor,
	    .Depth = desc.Depth,
	    .MotionVectors = desc.MotionVectors,
	    .Exposure = desc.Exposure,
	    .Normals = desc.Normals,
	    .Roughness = desc.Roughness,
	    .DiffuseAlbedo = desc.DiffuseAlbedo,
	    .SpecularAlbedo = desc.SpecularAlbedo,
	    .SpecularHitDistance = desc.SpecularHitDistance,
	    .RenderExtent = desc.RenderExtent,
	    .OutputExtent = desc.OutputExtent,
	    .FrameIndex = desc.FrameIndex,
	    .ResetRequested = historyInvalid,
	    .HistoryInvalid = historyInvalid,
	    .ResetReason = historyInvalid ? "Temporal history invalid or unavailable" : "",
	    .Camera = desc.Camera,
	    .TemporalData = desc.TemporalData,
	    .TemporalState = desc.TemporalState,
	    .MotionVectorConvention =
	        RayReconstructionMotionVectorConvention{
	            .Units = ERayReconstructionMotionVectorUnits::PixelDelta,
	            .Direction = ERayReconstructionMotionVectorDirection::CurrentMinusPrevious},
	    .DepthConvention = ERayReconstructionDepthConvention::ReversedDeviceDepth};
}
