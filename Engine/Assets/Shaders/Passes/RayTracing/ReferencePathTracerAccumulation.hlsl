Texture2D<float4> ReferencePathTracerSample;
RWTexture2D<float4> SceneColorTexture;
Texture2D<float4> PreviousReferencePathTracer;
RWTexture2D<float4> CurrentReferencePathTracer;
Texture2D<float4> ReferencePathTracerSampleValidity;
Texture2D<float2> GBufferMotionVector;

cbuffer ReferencePathTracerAccumulationConstants
{
	uint ReferencePathTracerSamplesPerFrame;
	uint ReferencePathTracerHistoryValid;
	uint ReferencePathTracerAccumulationPadding1;
	uint ReferencePathTracerAccumulationPadding2;
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	SceneColorTexture.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const float4 currentSample = ReferencePathTracerSample.Load(int3(pixelCoord, 0));
	const bool producerValid = ReferencePathTracerSampleValidity.Load(int3(pixelCoord, 0)).a > 0.5f;
	const float2 motionVector = GBufferMotionVector.Load(int3(pixelCoord, 0));
	const bool canReuseHistory = ReferencePathTracerHistoryValid != 0u && dot(motionVector, motionVector) <= 1.0e-6f;

	float3 previousRadiance = 0.0f.xxx;
	float previousSampleCount = 0.0f;
	if (canReuseHistory)
	{
		const float4 previous = PreviousReferencePathTracer.Load(int3(pixelCoord, 0));
		previousRadiance = previous.rgb;
		previousSampleCount = previous.a;
	}

	if (!producerValid)
	{
		CurrentReferencePathTracer[pixelCoord] = float4(previousRadiance, previousSampleCount);
		SceneColorTexture[pixelCoord] = float4(previousRadiance, currentSample.a);
		return;
	}

	const float currentSampleCount = float(max(ReferencePathTracerSamplesPerFrame, 1u));
	const float accumulatedSampleCount = previousSampleCount + currentSampleCount;
	const float3 accumulatedRadiance =
	    (previousRadiance * previousSampleCount + currentSample.rgb * currentSampleCount) / accumulatedSampleCount;

	CurrentReferencePathTracer[pixelCoord] = float4(accumulatedRadiance, accumulatedSampleCount);
	SceneColorTexture[pixelCoord] = float4(accumulatedRadiance, currentSample.a);
}
