#include "/Engine/Display/Exposure.hlsli"

Texture2D LuminanceMoments;
Texture2D PreviousExposureTexture;
RWTexture2D<float4> ExposureTexture;
RWTexture2D<float4> ExposureHistoryTexture;

cbuffer ExposureUniformData
{
	uint ExposureMode;
	uint ExposureHistoryValid;
	float FrameDeltaSeconds;
	float ManualExposure;
	float ExposureCompensation;
	float ExposureTargetLuminance;
	float ExposureMin;
	float ExposureMax;
	float ExposureAdaptationSpeedUp;
	float ExposureAdaptationSpeedDown;
	uint ExposurePadding0;
	uint ExposurePadding1;
};

[numthreads(1, 1, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	const float averageLuminance = Exposure::ResolveAverageLuminance(LuminanceMoments.Load(int3(0, 0, 0)).xy);

	const float targetExposure =
	    Exposure::ComputeExposure(
	        ExposureMode,
	        ManualExposure,
	        ExposureCompensation,
	        ExposureTargetLuminance,
	        ExposureMin,
	        ExposureMax,
	        averageLuminance);

	const float previousExposure = PreviousExposureTexture.Load(int3(0, 0, 0)).r;
	
	const float exposure =
	    Exposure::AdaptExposure(
	        ExposureMode,
	        ExposureHistoryValid != 0u,
	        previousExposure,
	        targetExposure,
	        FrameDeltaSeconds,
	        ExposureAdaptationSpeedUp,
	        ExposureAdaptationSpeedDown);

	const float4 exposurePayload = float4(exposure, averageLuminance, targetExposure, previousExposure);
	ExposureTexture[uint2(0u, 0u)] = exposurePayload;
	ExposureHistoryTexture[uint2(0u, 0u)] = exposurePayload;
}
