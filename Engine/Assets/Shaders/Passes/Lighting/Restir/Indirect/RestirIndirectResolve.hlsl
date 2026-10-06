
Texture2D<float4> CurrentReservoirSampleTexture;
Texture2D<float4> CurrentReservoirWeightTexture;
Texture2D SkyTexture;
SamplerState SamplerLinearClamp;

#include "/Engine/Lighting/RestirIndirectReservoir.hlsli"
RWTexture2D<float4> IndirectDiffuse;
RWTexture2D<float4> IndirectSpecular;
#include "/Engine/Lighting/RayReconstructionGuides.hlsli"

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	IndirectDiffuse.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
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
		if (RestirIndirectWriteReconstructionGuides != 0u)
		{
			RayReconstructionGuides::Clear(pixelCoord);
		}
		return;
	}

	if (RestirIndirectWriteReconstructionGuides != 0u)
	{
		RayReconstructionGuides::WriteSurface(pixelCoord, surface.GBuffer, surface.PathSurface.ViewDirWorld);
	}
	if (!RestirIndirectReservoir::IsValid(reservoir))
	{
		if (RestirIndirectWriteReconstructionGuides != 0u)
		{
			RayReconstructionGuides::ClearSpecularHitDistance(pixelCoord);
		}
		return;
	}

	RayTracingPathLighting::Result path =
	    RestirIndirectReservoir::EvaluateCandidate(surface, reservoir.Selected, SkyTexture, SamplerLinearClamp);
	const float reservoirWeight = RestirIndirectReservoir::GetFinalWeight(reservoir);
	if (RestirIndirectEvaluateDiffuse != 0u)
	{
		const float3 diffuse = path.DiffuseContribution * reservoirWeight;
		IndirectDiffuse[pixelCoord] = float4(diffuse, any(diffuse > 0.0f) ? 1.0f : 0.0f);
	}
	bool hasSpecular = false;
	if (RestirIndirectEvaluateSpecular != 0u)
	{
		const float3 specular = path.SpecularContribution * reservoirWeight;
		hasSpecular = any(specular > 0.0f);
		IndirectSpecular[pixelCoord] = float4(specular, hasSpecular ? 1.0f : 0.0f);
	}
	if (RestirIndirectWriteReconstructionGuides != 0u)
	{
		RayReconstructionGuides::WriteSpecularHitDistance(pixelCoord,
		                                                  path.FirstLighting.HitPositionWorld,
		                                                  surface.PathSurface.PositionWorld,
		                                                  hasSpecular && path.FirstLighting.Hit);
	}
}
