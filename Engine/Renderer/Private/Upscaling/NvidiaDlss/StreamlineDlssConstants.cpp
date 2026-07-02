#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssConstants.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineViewConstants.h"

namespace
{
	bool IsCurrentMinusPrevious(const UpscalerInputContract& inputContract) noexcept
	{
		return inputContract.MotionVectorConvention.Direction == EUpscalerMotionVectorDirection::CurrentMinusPrevious;
	}
}

void FillStreamlineConstants(sl::Constants& constants, const UpscalerInputContract& inputContract) noexcept
{
	FillStreamlineViewConstants(
	    constants,
	    StreamlineViewConstantsInput{
	        .Camera = inputContract.Camera,
	        .TemporalData = inputContract.TemporalData,
	        .TemporalState = inputContract.TemporalState,
	        .RenderExtent = inputContract.RenderExtent,
	        .MotionVectorsCurrentMinusPrevious = IsCurrentMinusPrevious(inputContract),
	        .ReversedDeviceDepth = inputContract.DepthConvention == EUpscalerDepthConvention::ReversedDeviceDepth,
	        .ResetRequested = inputContract.ResetRequested});
}
#endif
