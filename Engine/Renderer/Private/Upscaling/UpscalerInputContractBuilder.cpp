#include "../PCH.h"
#include "Upscaling/UpscalerInputContractBuilder.h"

UpscalerInputContract BuildUpscalerInputContract(const UpscalerInputContractBuildDesc& desc)
{
	const bool historyInvalid = !desc.TemporalState.HistoryValid;
	return UpscalerInputContract{
	    .ScalingInputColor = desc.ScalingInputColor,
	    .Depth = desc.Depth,
	    .MotionVectors = desc.MotionVectors,
	    .Exposure = desc.Exposure,
	    .ScalingOutputColor = desc.ScalingOutputColor,
	    .RenderExtent = desc.RenderExtent,
	    .OutputExtent = desc.OutputExtent,
	    .FrameIndex = desc.FrameIndex,
	    .HdrMetadataAvailable = false,
	    .ExposureRequired = desc.ExposureRequired,
	    .ResetRequested = historyInvalid,
	    .CameraCut = false,
	    .HistoryInvalid = historyInvalid,
	    .ResetReason = historyInvalid ? "Temporal history invalid or unavailable" : "",
	    .Camera = desc.Camera,
	    .TemporalData = desc.TemporalData,
	    .TemporalState = desc.TemporalState,
	    .MotionVectorConvention =
	        UpscalerMotionVectorConvention{
	            .Units = EUpscalerMotionVectorUnits::PixelDelta,
	            .Direction = EUpscalerMotionVectorDirection::CurrentMinusPrevious},
	    .DepthConvention = EUpscalerDepthConvention::ReversedDeviceDepth};
}
