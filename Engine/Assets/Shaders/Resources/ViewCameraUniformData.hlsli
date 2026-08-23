#pragma once

cbuffer ViewCamera
{
	row_major float4x4 ViewMTX;
	row_major float4x4 ProjectionMTX;
	row_major float4x4 ViewProjMTX;
	row_major float4x4 InvViewMTX;
	row_major float4x4 InvProjectionMTX;

	float3 Position;
	float NearZ;

	float FarZ;
	float3 Direction;
};
