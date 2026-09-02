#include "/Engine/Lighting/IndirectLightingOutputs.hlsli"
#include "/Engine/RayTracing/Shadows/RayTracedShadowVisibility.hlsli"
#include "/Engine/RayTracing/GBufferPathSurface.hlsli"
#include "/Engine/RayTracing/PathLighting.hlsli"
#include "/Engine/RayTracing/PathTracedLightingUniform.hlsli"

Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

void ClearPathTracedIndirectLightingPixel(uint2 pixelCoord)
{
	// Alpha marks that the path producer ran for this pixel. Reference accumulation
	// must distinguish a valid black/sky sample from a skipped producer pass.
	IndirectLightingOutputs::Clear(pixelCoord, true);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	IndirectDiffuse.GetDimensions(width, height);
	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const uint2 pixelCoord = dispatchThreadId.xy;
	const RayTracingGBufferPathSurface::Surface primarySurface = RayTracingGBufferPathSurface::Load(pixelCoord);
	if (!primarySurface.Valid)
	{
		ClearPathTracedIndirectLightingPixel(pixelCoord);
		return;
	}
	const RayTracingPathTrace::TraceSettings traceSettings =
	    RayTracingPathTrace::BuildSurfaceTraceSettings(PathTracedLightingNormalBias, PathTracedLightingMaxDistance);

	float3 indirectDiffuse = 0.0f.xxx;
	float3 indirectSpecular = 0.0f.xxx;
	const uint sampleCount = max(PathTracedLightingSamplesPerPixel, 1u);
	[loop]
	for (uint sampleIndex = 0u; sampleIndex < sampleCount; ++sampleIndex)
	{
		const RayTracingPathLighting::Result path =
		    RayTracingPathLighting::TraceSurfacePath(SkyTexture,
		                                             SamplerLinearClamp,
		                                             primarySurface.PathSurface,
		                                             pixelCoord,
		                                             sampleIndex,
		                                             RayTracingPathSampling::SpecularSampleModeStochasticGGX,
		                                             PathTracedLightingBounceCount,
		                                             traceSettings);
		if (path.PrimaryLobe == RayTracingPathSample::LobeDiffuse)
		{
			indirectDiffuse += path.FinalContribution;
		}
		else if (path.PrimaryLobe == RayTracingPathSample::LobeSpecular)
		{
			indirectSpecular += path.FinalContribution;
		}
	}

	const float invSampleCount = rcp(float(sampleCount));
	indirectDiffuse *= invSampleCount;
	indirectSpecular *= invSampleCount;
	IndirectDiffuse[pixelCoord] = float4(indirectDiffuse, 1.0f);
	IndirectSpecular[pixelCoord] = float4(indirectSpecular, 1.0f);
}
