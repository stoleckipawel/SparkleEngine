#include "/Engine/Resources/ViewCameraUniformData.hlsli"

#include "/Engine/Passes/GBuffer/SceneDepthUtils.hlsli"

Texture2D<float> GBufferDeviceZ;
RWTexture2D<float> SceneDepth;

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneDepth.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float deviceZ = GBufferDeviceZ.Load(int3(dispatchThreadId.xy, 0)).r;
	SceneDepth[dispatchThreadId.xy] = SceneDepthUtils::LinearizeDeviceZ(deviceZ, NearZ, FarZ);
}
