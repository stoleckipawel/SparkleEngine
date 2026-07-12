#pragma once

#include "Geometry/ScreenSpace.hlsli"

namespace MotionVectors
{
	float2 JitterNdcToViewportPixels(const float2 jitterNdc, const float2 viewportSize)
	{
		return float2(jitterNdc.x, -jitterNdc.y) * (0.5f * viewportSize);
	}

	float2 ClipToViewportPixels(const float4 clipPosition, const float2 viewportSize)
	{
		const float2 ndc = clipPosition.xy / clipPosition.w;
		const float2 viewportUv = float2(ndc.x, -ndc.y) * 0.5f + 0.5f;
		return viewportUv * viewportSize;
	}

	float2 Compute(const float4 currentClipPosition, const float4 previousClipPosition, const float2 viewportSize)
	{
		const bool bPreviousFrameValid = (HistoryValid != 0u);
		if (!bPreviousFrameValid || currentClipPosition.w <= 1e-6f || previousClipPosition.w <= 1e-6f)
		{
			return float2(0.0f, 0.0f);
		}

		const float2 currentPixels = ClipToViewportPixels(currentClipPosition, viewportSize);
		const float2 prevPixels = ClipToViewportPixels(previousClipPosition, viewportSize);

		return currentPixels - prevPixels;
	}

	float2 ComputeRaster(
		const float2 rasterPositionPixels,
		const float4 previousClipPosition,
		const float2 viewportSize)
	{
		if (HistoryValid == 0u || previousClipPosition.w <= 1.0e-6f)
		{
			return float2(0.0f, 0.0f);
		}

		// SV_Position addresses the jittered render grid. Motion vectors are declared
		// unjittered to Streamline, so remove the current sample offset explicitly
		// instead of relying on a second interpolated copy of current clip position.
		const float2 currentPixels = rasterPositionPixels - JitterNdcToViewportPixels(JitterCurrent, viewportSize);
		const float2 previousPixels = ClipToViewportPixels(previousClipPosition, viewportSize);
		return currentPixels - previousPixels;
	}

	float2 ReprojectToPreviousPixelCenter(const uint2 pixelCoord, const float2 motionPixels, const float2 viewportSize)
	{
		// Reservoir history lives on jittered pixel grids while motion excludes
		// jitter. Move from the current grid to the previous grid after applying
		// geometric motion.
		const float2 jitterDeltaPixels = JitterNdcToViewportPixels(JitterPrevious - JitterCurrent, viewportSize);
		return (float2(pixelCoord) + 0.5f) - motionPixels + jitterDeltaPixels;
	}

	float2 ComputeCameraRotation(
		const uint2 pixelCoord,
		const float3 currentDirectionWorld,
		const float2 viewportSize)
	{
		const float2 currentNdc = PixelCenterToUnjitteredNdc(pixelCoord);
		const float4 currentClipPosition = float4(currentNdc, 1.0f, 1.0f);
		const float4 previousDirectionView = mul(float4(currentDirectionWorld, 0.0f), PrevViewMTX);
		const float4 previousClipPosition = mul(previousDirectionView, PrevProjectionMTX);
		return Compute(currentClipPosition, previousClipPosition, viewportSize);
	}
}  // namespace MotionVectors
