#include "/Engine/Passes/PostProcessing/Exposure/ExposureHistogram.hlsli"

RWStructuredBuffer<uint> HistogramCounts;

[numthreads(256, 1, 1)]
void main(uint3 groupThreadId : SV_GroupThreadID)
{
	HistogramCounts[groupThreadId.x] = 0u;
	HistogramCounts[groupThreadId.x + 256u] = 0u;
}
