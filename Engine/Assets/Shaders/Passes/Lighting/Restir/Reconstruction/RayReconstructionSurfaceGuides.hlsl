#include "/Engine/Lighting/RayReconstructionGuides.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	RayReconstructionRoughness.GetDimensions(width, height);
	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}
	const GBufferData gBuffer = LoadGBuffer(pixelCoord);
	if (IsSkyPixel(gBuffer.SceneDepth))
	{
		RayReconstructionGuides::Clear(pixelCoord);
		return;
	}
	const float3 positionWorld = ReconstructGBufferWorldPosition(pixelCoord, gBuffer.SceneDepth, InvViewMTX, InvProjectionMTX);
	const float3 cameraToSurface = positionWorld - Position;
	const float viewDistance = length(cameraToSurface);
	const float3 viewDirWorld = viewDistance > 1.0e-5f ? -cameraToSurface / viewDistance : gBuffer.NormalWorld;
	RayReconstructionGuides::WriteSurface(pixelCoord, gBuffer, viewDirWorld);
	RayReconstructionGuides::ClearSpecularHitDistance(pixelCoord);
}
