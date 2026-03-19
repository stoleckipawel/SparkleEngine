#pragma once

struct PerViewCameraConstantBufferData
{
	float3 Position;
	float NearZ;

	float FarZ;
	float3 Direction;
};