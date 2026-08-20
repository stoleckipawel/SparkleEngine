#pragma once

class RenderScene;
struct RayTracingCapabilityReport;
struct RayTracedShadowSettings;

struct RayTracingPassContext
{
	RenderScene* Scene = nullptr;
	const RayTracingCapabilityReport* CapabilityReport = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
	bool ShadowsEnabled = true;
};
