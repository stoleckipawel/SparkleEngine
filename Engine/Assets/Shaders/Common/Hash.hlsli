#pragma once

float3 HashIdColor(uint id0, uint id1)
{
	const uint seed = id0 * 1664525u + id1 * 1013904223u + 0x9E3779B9u;
	const uint r = (seed >> 0u) & 255u;
	const uint g = (seed >> 8u) & 255u;
	const uint b = (seed >> 16u) & 255u;
	return 0.15f.xxx + 0.85f * (float3(r, g, b) / 255.0f);
}
