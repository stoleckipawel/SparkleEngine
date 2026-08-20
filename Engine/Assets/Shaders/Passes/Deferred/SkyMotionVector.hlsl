#include "Resources/ViewUniformData.hlsli"

#include "Geometry/ScreenSpace.hlsli"
#include "Passes/Deferred/MotionVector.hlsli"
#include "Passes/Deferred/SceneDepthUtils.hlsli"

Texture2D<float> GBufferDeviceZ;
RWTexture2D<float2> GBufferMotionVector;

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0u;
	uint height = 0u;
	GBufferMotionVector.GetDimensions(width, height);

	const uint2 pixelCoord = dispatchThreadId.xy;
	if (pixelCoord.x >= width || pixelCoord.y >= height)
	{
		return;
	}

	const float deviceZ = GBufferDeviceZ.Load(int3(pixelCoord, 0));
	if (!SceneDepthUtils::IsSkyDeviceZ(deviceZ))
	{
		return;
	}

	const float3 directionWorld = ComputeSkyViewDirectionWorld(pixelCoord);
	GBufferMotionVector[pixelCoord] = MotionVectors::ComputeCameraRotation(pixelCoord, directionWorld, ViewportSize);
}
