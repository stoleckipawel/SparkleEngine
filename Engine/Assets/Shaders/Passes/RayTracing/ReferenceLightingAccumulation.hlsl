Texture2D<float4> ReferenceLightingSample;
RWTexture2D<float4> SceneColorTexture;
Texture2D<float4> PreviousReferenceLighting;
RWTexture2D<float4> CurrentReferenceLighting;
Texture2D<float4> ReferenceSampleValidity;
Texture2D<float2> GBufferMotionVector;

cbuffer ReferenceLightingAccumulationUniformData
{
	uint ReferenceLightingSamplesPerFrame;
	uint ReferenceLightingHistoryValid;
	uint ReferenceLightingAccumulationPadding1;
	uint ReferenceLightingAccumulationPadding2;
};

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	SceneColorTexture.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const float4 currentSample = ReferenceLightingSample.Load(int3(pixelCoord, 0));
	const bool producerValid = ReferenceSampleValidity.Load(int3(pixelCoord, 0)).a > 0.5f;
	const bool currentSampleFinite = all(isfinite(currentSample.rgb));
	const float2 motionVector = GBufferMotionVector.Load(int3(pixelCoord, 0));
	const bool canReuseHistory = ReferenceLightingHistoryValid != 0u && dot(motionVector, motionVector) <= 1.0e-6f;

	float3 previousRadiance = 0.0f.xxx;
	float previousSampleCount = 0.0f;
	if (canReuseHistory)
	{
		const float4 previous = PreviousReferenceLighting.Load(int3(pixelCoord, 0));
		if (all(isfinite(previous)) && previous.a >= 0.0f)
		{
			previousRadiance = previous.rgb;
			previousSampleCount = previous.a;
		}
	}

	if (!producerValid || !currentSampleFinite)
	{
		CurrentReferenceLighting[pixelCoord] = float4(previousRadiance, previousSampleCount);
		const float3 fallbackRadiance =
		    previousSampleCount > 0.0f ? previousRadiance : (currentSampleFinite ? max(currentSample.rgb, 0.0f.xxx) : 0.0f.xxx);
		SceneColorTexture[pixelCoord] = float4(fallbackRadiance, currentSample.a);
		return;
	}

	const float currentSampleCount = float(max(ReferenceLightingSamplesPerFrame, 1u));
	const float accumulatedSampleCount = previousSampleCount + currentSampleCount;
	const float3 accumulatedRadiance = (previousRadiance * previousSampleCount + currentSample.rgb * currentSampleCount) / accumulatedSampleCount;

	CurrentReferenceLighting[pixelCoord] = float4(accumulatedRadiance, accumulatedSampleCount);
	SceneColorTexture[pixelCoord] = float4(accumulatedRadiance, currentSample.a);
}
