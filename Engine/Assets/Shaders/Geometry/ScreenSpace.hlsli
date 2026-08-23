#ifndef SPARKLE_GEOMETRY_SCREEN_SPACE_HLSLI
#define SPARKLE_GEOMETRY_SCREEN_SPACE_HLSLI

#include "/Engine/Resources/ViewUniformData.hlsli"
#include "/Engine/Resources/ViewCameraUniformData.hlsli"
#include "/Engine/Resources/ViewTemporalUniformData.hlsli"
float2 PixelCenterToNdc(uint2 pixelCoord)
{
	const float2 uv = (float2(pixelCoord) + 0.5f) * ViewportSizeInv;
	return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

float2 PixelCenterToUnjitteredNdc(uint2 pixelCoord)
{
	return PixelCenterToNdc(pixelCoord) - CurrentJitterNdc;
}

float3 ComputeSkyViewDirectionWorld(uint2 pixelCoord)
{
	const float2 ndc = PixelCenterToUnjitteredNdc(pixelCoord);
	const float4 positionClip = float4(ndc, 1.0f, 1.0f);
	const float4 positionView = mul(positionClip, InvProjectionMTX);
	const float3 viewDirection = normalize(positionView.xyz / max(positionView.w, 1.0e-6f));
	return normalize(mul(float4(viewDirection, 0.0f), InvViewMTX).xyz);
}

#endif
