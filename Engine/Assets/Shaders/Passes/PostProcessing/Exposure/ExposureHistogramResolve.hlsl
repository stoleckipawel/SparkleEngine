#include "/Engine/Passes/PostProcessing/Exposure/ExposureHistogram.hlsli"

StructuredBuffer<uint> HistogramCounts;

RWTexture2D<float4> LuminanceMomentsOutput;

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint sampleCount = 0u;
	[loop]
	for (uint bin = 0u; bin < ExposureHistogram::BinCount; ++bin)
	{
		sampleCount += HistogramCounts[bin];
	}

	const float lowCount = float(sampleCount) * ExposureHistogram::LowPercentile;
	const float highCount = float(sampleCount) * ExposureHistogram::HighPercentile;
	float cumulativeCount = 0.0f;
	float weightedLuminance = 0.0f;
	float retainedCount = 0.0f;
	[loop]
	for (uint bin = 0u; bin < ExposureHistogram::BinCount; ++bin)
	{
		const float nextCount = cumulativeCount + float(HistogramCounts[bin]);
		const float weight = max(min(nextCount, highCount) - max(cumulativeCount, lowCount), 0.0f);
		weightedLuminance += ExposureHistogram::BinLuminance(bin) * weight;
		retainedCount += weight;
		cumulativeCount = nextCount;
	}

	const float averageLuminance = retainedCount > 0.0f ? max(weightedLuminance / retainedCount, Exposure::MinimumMeteredLuminance) : Exposure::MinimumMeteredLuminance;
	LuminanceMomentsOutput[uint2(0u, 0u)] = float4(log(averageLuminance), sampleCount != 0u ? 1.0f : 0.0f, 0.0f, 0.0f);
}
