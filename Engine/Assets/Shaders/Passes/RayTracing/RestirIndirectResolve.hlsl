#include "Lighting/RestirIndirectLightingUniform.hlsli"

Texture2D<float4> CurrentReservoirSampleTexture;
Texture2D<float4> CurrentReservoirWeightTexture;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

#include "Lighting/RestirIndirectReservoir.hlsli"
#include "Lighting/IndirectLightingOutputs.hlsli"
#include "Lighting/RayReconstructionGuides.hlsli"

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	IndirectDiffuse.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	uint guideWidth = 0u;
	uint guideHeight = 0u;
	RayReconstructionRoughness.GetDimensions(guideWidth, guideHeight);
	const bool writeRayReconstructionGuides = guideWidth == width && guideHeight == height;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}
	const RestirIndirectReservoir::Surface surface = RestirIndirectReservoir::LoadSurface(pixelCoord);
	const RestirIndirectReservoir::Reservoir reservoir =
	    RestirIndirectReservoir::UnpackReservoir(CurrentReservoirSampleTexture.Load(int3(pixelCoord, 0)),
	                                             CurrentReservoirWeightTexture.Load(int3(pixelCoord, 0)));

	if (!surface.Valid)
	{
		IndirectLightingOutputs::Clear(pixelCoord, false);
		if (writeRayReconstructionGuides)
		{
			RayReconstructionGuides::Clear(pixelCoord);
		}
		return;
	}

	if (writeRayReconstructionGuides)
	{
		RayReconstructionGuides::WriteSurface(pixelCoord, surface.GBuffer, surface.PathSurface.ViewDirWorld);
	}
	if (!RestirIndirectReservoir::IsValid(reservoir))
	{
		IndirectLightingOutputs::ClearRadiance(pixelCoord);
		if (writeRayReconstructionGuides)
		{
			RayReconstructionGuides::ClearSpecularHitDistance(pixelCoord);
		}
		return;
	}

	RayTracingPathLighting::Result path =
	    RestirIndirectReservoir::EvaluateCandidate(surface, reservoir.Selected, SkyTexture, SamplerLinearClamp);
	path.FinalContribution *= RestirIndirectReservoir::GetFinalWeight(reservoir);
	const bool diffuseSelected = path.PrimaryLobe == RayTracingPathSample::LobeDiffuse;
	const bool specularSelected = path.PrimaryLobe == RayTracingPathSample::LobeSpecular;
	const float3 diffuse = diffuseSelected ? path.FinalContribution : 0.0f.xxx;
	const float3 specular = specularSelected ? path.FinalContribution : 0.0f.xxx;
	IndirectDiffuse[pixelCoord] = float4(diffuse, diffuseSelected ? 1.0f : 0.0f);
	IndirectSpecular[pixelCoord] = float4(specular, specularSelected ? 1.0f : 0.0f);
	if (writeRayReconstructionGuides)
	{
		RayReconstructionGuides::WriteSpecularHitDistance(pixelCoord, path, surface.PathSurface.PositionWorld, specularSelected);
	}
}
