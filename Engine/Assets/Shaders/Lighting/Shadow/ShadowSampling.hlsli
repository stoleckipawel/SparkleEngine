#pragma once

#include "Lighting/Shadow/ShadowMaps.hlsli"

namespace Shadow
{
	float SampleShadowMapVisibility(float2 uv, float receiverDepth, float shadowMapSize, uint mapIndex)
	{
		const float texelSize = 1.0f / shadowMapSize;
		const float2 clampedUv = clamp(uv, texelSize * 0.5f, 1.0f - texelSize * 0.5f);
		const float2 texelPhase = clampedUv * shadowMapSize + 0.5f;
		const float2 snappedTexel = floor(texelPhase);
		const float2 bilinearWeights = frac(texelPhase);
		const float2 gatherUv = clamp(snappedTexel * texelSize, texelSize, 1.0f - texelSize);

		const float4 gatheredDepths = GatherShadowDepth(gatherUv, mapIndex, int2(0, 0));
		const float4 visibility = float4(
		    receiverDepth >= gatheredDepths.x ? 1.0f : 0.0f,
		    receiverDepth >= gatheredDepths.y ? 1.0f : 0.0f,
		    receiverDepth >= gatheredDepths.z ? 1.0f : 0.0f,
		    receiverDepth >= gatheredDepths.w ? 1.0f : 0.0f);

		const float topRow = lerp(visibility.w, visibility.z, bilinearWeights.x);
		const float bottomRow = lerp(visibility.x, visibility.y, bilinearWeights.x);
		return lerp(topRow, bottomRow, bilinearWeights.y);
	}

	float SampleDirectionalShadowVisibility(float2 uv, float receiverDepth, float shadowMapSize, uint lightIndex, uint cascadeIndex)
	{
		return SampleShadowMapVisibility(uv, receiverDepth, shadowMapSize, GetShadowMapIndex(lightIndex, cascadeIndex));
	}
}  // namespace Shadow