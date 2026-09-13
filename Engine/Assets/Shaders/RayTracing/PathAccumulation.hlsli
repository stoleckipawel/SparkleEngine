#pragma once

namespace PathAccumulation
{
	void AddSample(inout float3 mean, inout float3 m2, uint priorCount, float3 sample)
	{
		const float count = (float)(priorCount + 1u);
		const float3 delta = sample - mean;
		mean += delta / count;
		m2 += delta * (sample - mean);
	}

	float3 SampleVariance(float3 m2, uint count)
	{
		return count > 1u ? m2 / (float)(count - 1u) : 0.0f.xxx;
	}

	float3 StandardError(float3 m2, uint count)
	{
		return count > 1u ? sqrt(max(SampleVariance(m2, count), 0.0f.xxx) / (float)count) : 0.0f.xxx;
	}
}
