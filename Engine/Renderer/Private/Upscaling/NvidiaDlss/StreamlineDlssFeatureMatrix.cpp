#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssFeatureMatrix.h"

DlssFeatureMatrix CreateUnavailableStreamlineDlssFeatureMatrix(std::string_view reason)
{
	DlssFeatureMatrix matrix;
	matrix.Entries = {
	    DlssFeatureMatrixEntry{
	        .Feature = EDlssFeatureKind::SuperResolution,
	        .State = EDlssFeatureState::Unavailable,
	        .QualityModes = "NativeAA, Quality, Balanced, Performance, UltraPerformance",
	        .ModelPresetRecommendation = "SDK queried preset recommendation required.",
	        .RequiredResources = "HUD-less color, depth, motion vectors, exposure or auto-exposure, final output",
	        .Reason = std::string(reason)},
	    DlssFeatureMatrixEntry{
	        .Feature = EDlssFeatureKind::NativeAA,
	        .State = EDlssFeatureState::Unavailable,
	        .QualityModes = "NativeAA",
	        .ModelPresetRecommendation = "Reuse Super Resolution preset recommendation at render extent equal to output extent.",
	        .RequiredResources = "Same as Super Resolution; render extent must equal output extent",
	        .Reason = std::string(reason)}};
	return matrix;
}

DlssFeatureMatrix CreateStreamlineDlssFeatureMatrix(bool superResolutionSupported, std::string_view reason)
{
	DlssFeatureMatrix matrix = CreateUnavailableStreamlineDlssFeatureMatrix(reason);
	for (DlssFeatureMatrixEntry& entry : matrix.Entries)
	{
		if (entry.Feature == EDlssFeatureKind::SuperResolution || entry.Feature == EDlssFeatureKind::NativeAA)
		{
			entry.State = superResolutionSupported ? EDlssFeatureState::Available : EDlssFeatureState::Unavailable;
			entry.Supported = superResolutionSupported;
			entry.Reason = std::string(reason);
		}
	}
	return matrix;
}
