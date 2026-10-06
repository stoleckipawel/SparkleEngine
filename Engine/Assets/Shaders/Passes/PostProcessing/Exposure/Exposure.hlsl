#include "/Engine/Display/Exposure.hlsli"
#include "/Engine/Resources/FrameUniformData.hlsli"

Texture2D LuminanceMoments;
Texture2D PreviousExposureTexture;
RWTexture2D<float4> ExposureTexture;
RWTexture2D<float4> ExposureHistoryTexture;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	const float2 moments = LuminanceMoments.Load(int3(0, 0, 0)).xy;
	const float averageLuminance = Exposure::ResolveAverageLuminance(moments);
	const float4 previousPayload = ExposureHistoryValid != 0u ? PreviousExposureTexture.Load(int3(0, 0, 0)) : 0.0f.xxxx;
	const bool historyValid = ExposureHistoryValid != 0u && previousPayload.a == float(ExposureMode);

	float targetExposure = Exposure::ComputeExposure(
	    ExposureMode,
	    ManualExposure,
	    ExposureCompensation,
	    ExposureTargetLuminance,
	    ExposureMin,
	    ExposureMax,
	    averageLuminance);

	if (ExposureMode == Exposure::ExposureModeAutomatic && moments.y <= 0.0f)
	{
		// An invalid HDR frame cannot drive adaptation; the first frame uses neutral exposure.
		targetExposure = clamp(
		    historyValid && isfinite(previousPayload.r) && previousPayload.r > 0.0f ? previousPayload.r : 1.0f,
		    ExposureMin,
		    ExposureMax);
	}

	const float exposure = clamp(
	    Exposure::AdaptExposure(
	        ExposureMode,
	        historyValid,
	        previousPayload.r,
	        targetExposure,
	        DeltaTimeSeconds,
	        ExposureAdaptationSpeedUp,
	        ExposureAdaptationSpeedDown),
	    ExposureMin,
	    ExposureMax);

	const float4 exposurePayload = float4(exposure, averageLuminance, targetExposure, float(ExposureMode));
	ExposureTexture[uint2(0u, 0u)] = exposurePayload;
	ExposureHistoryTexture[uint2(0u, 0u)] = exposurePayload;
}
