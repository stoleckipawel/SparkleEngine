#pragma once

#include "Resources/ConstantBuffers.hlsli"

// -----------------------------------------------------------------------------
// Position Transforms
// -----------------------------------------------------------------------------

float4 PositionLocalToWorld(float4 localPosition)
{
	return mul(localPosition, WorldMTX);
}

float4 PositionWorldToView(float4 worldPosition)
{
	return mul(worldPosition, ViewMTX);
}

float4 PositionViewToClip(float4 viewPosition)
{
	return mul(viewPosition, ProjectionMTX);
}

float4 PositionLocalToClip(float4 localPosition)
{
	const float4 worldPosition = PositionLocalToWorld(localPosition);
	const float4 viewPosition = PositionWorldToView(worldPosition);
	return PositionViewToClip(viewPosition);
}

float4 PositionWorldToClip(float4 worldPosition)
{
	return mul(worldPosition, ViewProjMTX);
}

// -----------------------------------------------------------------------------
// Normal/Tangent Transforms
// -----------------------------------------------------------------------------

// Transform normal from local to world space (handles non-uniform scale)
float3 NormalLocalToWorld(float3 normalLocal)
{
	return normalize(mul(normalLocal, WorldInvTransposeMTX));
}

// Transform tangent from local to world space (direction only)
float4 TangentLocalToWorld(float4 tangentLocal)
{
	const float3 worldTangent = mul(tangentLocal.xyz, (float3x3) WorldMTX);
	return float4(worldTangent, tangentLocal.w);  // Preserve handedness
}

// Compute bitangent from normal and tangent (using handedness)
float3 ComputeBitangent(float3 normalWorld, float4 tangentWorld)
{
	return tangentWorld.w * normalize(cross(normalWorld, tangentWorld.xyz));
}

// TBN Normal Transform
float3 TransformNormalToWorld(float3 normalTangent, float3 vertexNormalWorld, float3 vertexTangentWorld, float3 vertexBitangentWorld)
{
	const float3x3 TBN = float3x3(vertexTangentWorld, vertexBitangentWorld, vertexNormalWorld);
	return mul(normalTangent, TBN);
}

// -----------------------------------------------------------------------------
// Rotation Helper
// -----------------------------------------------------------------------------

float3 Rotate(float3 v, float3 axis, float angle)
{
	const float c = cos(angle);
	const float s = sin(angle);
	const float oneMinusC = 1.0f - c;

	const float3 a = normalize(axis);

	const float3x3 rotMtx = float3x3(
	    c + a.x * a.x * oneMinusC,
	    a.x * a.y * oneMinusC - a.z * s,
	    a.x * a.z * oneMinusC + a.y * s,
	    a.y * a.x * oneMinusC + a.z * s,
	    c + a.y * a.y * oneMinusC,
	    a.y * a.z * oneMinusC - a.x * s,
	    a.z * a.x * oneMinusC - a.y * s,
	    a.z * a.y * oneMinusC + a.x * s,
	    c + a.z * a.z * oneMinusC);

	return mul(v, rotMtx);
}