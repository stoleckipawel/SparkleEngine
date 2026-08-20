#ifndef SPARKLE_GBUFFER_PATH_SURFACE_HLSLI
#define SPARKLE_GBUFFER_PATH_SURFACE_HLSLI

#include "Resources/ViewCameraUniformData.hlsli"

#include "Passes/Deferred/GBufferUtils.hlsli"
#include "RayTracing/PathSurface.hlsli"

namespace RayTracingGBufferPathSurface
{
	struct Surface
	{
		bool Valid;
		GBufferData GBuffer;
		RayTracingPathSurface PathSurface;
		float ViewDistance;
	};

	Surface Load(uint2 pixelCoord)
	{
		Surface surface = (Surface)0;
		surface.GBuffer = LoadGBuffer(pixelCoord);
		if (IsSkyPixel(surface.GBuffer.SceneDepth))
		{
			return surface;
		}

		const float3 positionWorld = ReconstructGBufferWorldPosition(pixelCoord, surface.GBuffer.SceneDepth, InvViewMTX, InvProjectionMTX);
		const float3 cameraToSurface = positionWorld - Position;
		surface.ViewDistance = length(cameraToSurface);
		const float3 viewDirWorld = surface.ViewDistance > 1.0e-5f ? -cameraToSurface / surface.ViewDistance : surface.GBuffer.NormalWorld;
		surface.PathSurface = BuildPrimaryRayTracingPathSurface(positionWorld,
		                                                        surface.GBuffer.NormalWorld,
		                                                        viewDirWorld,
		                                                        surface.GBuffer.BaseColor,
		                                                        surface.GBuffer.Roughness,
		                                                        surface.GBuffer.Metallic,
		                                                        surface.GBuffer.DielectricF0);
		surface.Valid = true;
		return surface;
	}
}

#endif
