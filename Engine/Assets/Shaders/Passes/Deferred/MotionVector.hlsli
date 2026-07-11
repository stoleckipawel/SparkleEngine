#pragma once

#include "Geometry/ScreenSpace.hlsli"

namespace MotionVectors
{
	float2 ClipToViewportPixels(const float4 clipPosition, const float2 viewportSize)
	{
		const float2 ndc = clipPosition.xy / clipPosition.w;
		const float2 viewportUv = float2(ndc.x, -ndc.y) * 0.5f + 0.5f;
		return viewportUv * viewportSize;
	}

	float2 Compute(const float4 currentClipPosition, const float4 previousClipPosition, const float2 viewportSize)
	{
		const bool bPreviousFrameValid = (HistoryValid != 0u);
		if (!bPreviousFrameValid || abs(currentClipPosition.w) <= 1e-6f || abs(previousClipPosition.w) <= 1e-6f)
		{
			return float2(0.0f, 0.0f);
		}

		const float2 currentPixels = ClipToViewportPixels(currentClipPosition, viewportSize);
		const float2 prevPixels = ClipToViewportPixels(previousClipPosition, viewportSize);

		return currentPixels - prevPixels;
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
