#pragma once

class RenderRayTracingScene;
struct RayTracingCapabilityReport;
struct RayTracedShadowSettings;

struct RenderRayTracingPassServices
{
	RenderRayTracingScene* Scene = nullptr;
	const RayTracingCapabilityReport* CapabilityReport = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
	bool ShadowsEnabled = true;
};
