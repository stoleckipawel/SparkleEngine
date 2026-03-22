#pragma once

struct PerViewCameraConstantBufferData
{
	row_major float4x4 ViewMTX;
	row_major float4x4 ProjectionMTX;
	row_major float4x4 ViewProjMTX;

	float3 Position;
	float NearZ;

	float FarZ;
	float3 Direction;
};
