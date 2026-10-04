#pragma once

namespace RayTracingPathSample
{
	static const uint LobeDiffuse = 1u;
	static const uint LobeSpecular = 2u;

	struct DirectionSample
	{
		float3 DirectionWorld;
		float PdfW;
		float3 DiffuseThroughput;
		float3 SpecularThroughput;
		bool Delta;
		bool HasSupport;
	};

	struct LightingResult
	{
		bool Hit;
		float3 IncidentRadiance;
		float3 HitPositionWorld;
	};
}
