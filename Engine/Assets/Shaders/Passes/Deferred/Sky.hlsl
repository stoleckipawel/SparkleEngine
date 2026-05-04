#include "Resources/ConstantBuffers.hlsli"

RWTexture2D<float4> SceneColorTexture;
Texture2D GBufferDeviceZ;
TextureCube SkyTexture;
SamplerState SamplerLinearClamp;

float3 ComputeSkyDirection(uint2 pixelCoord)
{
	const float2 uv = (float2(pixelCoord) + 0.5f) * ViewportSizeInv;
	const float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
	const float4 positionClip = float4(ndc, 1.0f, 1.0f);
	const float4 positionView = mul(positionClip, Camera.InvProjectionMTX);
	const float3 viewDirection = normalize(positionView.xyz / max(positionView.w, 1.0e-6f));
	return normalize(mul(float4(viewDirection, 0.0f), Camera.InvViewMTX).xyz);
}

[numthreads(8, 8, 1)] void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint width = 0;
	uint height = 0;
	SceneColorTexture.GetDimensions(width, height);

	if (dispatchThreadId.x >= width || dispatchThreadId.y >= height)
	{
		return;
	}

	const float deviceZ = GBufferDeviceZ.Load(int3(dispatchThreadId.xy, 0)).r;
	if (deviceZ < 0.999999f)
	{
		return;
	}

	const float3 worldDirection = ComputeSkyDirection(dispatchThreadId.xy);
	const float3 skyColor = SkyTexture.SampleLevel(SamplerLinearClamp, worldDirection, 0.0f).rgb;
	SceneColorTexture[dispatchThreadId.xy] = float4(skyColor, 1.0f);
}