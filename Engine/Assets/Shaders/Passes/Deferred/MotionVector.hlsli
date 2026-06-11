#pragma once

namespace MotionVectors
{
	float2 Compute(const float4 currentClipPosition, const float4 previousClipPosition, const float2 viewportSize)
	{
		const bool bPreviousFrameValid = (HistoryValid != 0u);
		if (!bPreviousFrameValid || abs(previousClipPosition.w) <= 1e-6f)
		{
			return float2(0.0f, 0.0f);
		}

		const float2 currentPixels = currentClipPosition.xy;
		const float2 prevNdc = previousClipPosition.xy / previousClipPosition.w;
		const float2 prevNdcFlipped = float2(prevNdc.x, -prevNdc.y);
		const float2 prevViewportUv = prevNdcFlipped * 0.5f + 0.5f;
		const float2 prevPixels = prevViewportUv * viewportSize;

		return currentPixels - prevPixels;
	}
}  // namespace MotionVectors
