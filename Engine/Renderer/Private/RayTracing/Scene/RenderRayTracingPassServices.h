#pragma once

class RenderRayTracingScene;
struct RayTracingCapabilityReport;
struct RayTracedShadowSettings;
struct IndirectDiffuseSettings;
struct IndirectSpecularSettings;

struct RenderRayTracingPassServices
{
	RenderRayTracingScene* Scene = nullptr;
	const RayTracingCapabilityReport* CapabilityReport = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
	const IndirectDiffuseSettings* IndirectDiffuseSettings = nullptr;
	const IndirectSpecularSettings* IndirectSpecularSettings = nullptr;
};
