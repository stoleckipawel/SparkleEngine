#ifndef SPARKLE_DISPLAY_EXPOSURE_HLSLI
#define SPARKLE_DISPLAY_EXPOSURE_HLSLI

#include "/Engine/Common/Color.hlsli"

namespace Exposure
{
	static const uint ExposureModeManual = 0u;
	static const uint ExposureModeAutomatic = 1u;

	static const uint ExposureMeteringParallelReduction = 0u;
	static const uint ExposureMeteringDownsamplePyramid = 1u;

	static const float MinimumMeteredLuminance = 1.0e-4f;

	float2 BuildLogLuminanceMoment(float3 linearHdrColor)
	{
		const float luminance = max(CommonColor::LuminanceRec709(CommonColor::ClampPositive(linearHdrColor)), MinimumMeteredLuminance);
		return float2(log(luminance), 1.0f);
	}

	float ResolveAverageLuminance(float2 moments)
	{
		const float sampleCount = max(moments.y, 1.0f);
		return exp(moments.x / sampleCount);
	}

	float ComputeExposure(uint exposureMode,
	                      float manualExposure,
	                      float exposureCompensation,
	                      float targetLuminance,
	                      float minExposure,
	                      float maxExposure,
	                      float sceneAverageLuminance)
	{
		const float compensation = exp2(clamp(exposureCompensation, -16.0f, 16.0f));
		const float unclampedExposure = exposureMode == ExposureModeManual
		    ? max(manualExposure, 0.0f) * compensation
		    : (max(targetLuminance, MinimumMeteredLuminance) / max(sceneAverageLuminance, MinimumMeteredLuminance)) * compensation;
		const float safeMinExposure = max(minExposure, 0.0f);
		const float safeMaxExposure = max(maxExposure, safeMinExposure);
		return clamp(unclampedExposure, safeMinExposure, safeMaxExposure);
	}

	float AdaptExposure(uint exposureMode,
	                    bool exposureHistoryValid,
	                    float previousExposure,
	                    float targetExposure,
	                    float frameDeltaSeconds,
	                    float adaptationSpeedUp,
	                    float adaptationSpeedDown)
	{
		if (exposureMode == ExposureModeManual || !exposureHistoryValid)
		{
			return targetExposure;
		}

		const float safePreviousExposure = max(previousExposure, MinimumMeteredLuminance);
		const float safeTargetExposure = max(targetExposure, MinimumMeteredLuminance);
		const float adaptationSpeed = max(safeTargetExposure > safePreviousExposure ? adaptationSpeedUp : adaptationSpeedDown, 0.0f);
		const float adaptationT = saturate(1.0f - exp(-max(frameDeltaSeconds, 0.0f) * adaptationSpeed));
		const float previousExposureEv = log2(safePreviousExposure);
		const float targetExposureEv = log2(safeTargetExposure);
		return exp2(lerp(previousExposureEv, targetExposureEv, adaptationT));
	}
}

#endif
