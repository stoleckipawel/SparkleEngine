#pragma once

class RenderRayTracingScene;
struct RayTracedShadowSettings;
struct IndirectDiffuseSettings;
struct IndirectSpecularSettings;

struct RenderRayTracingPassServices
{
	RenderRayTracingScene* Scene = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
	const IndirectDiffuseSettings* IndirectDiffuseSettings = nullptr;
	const IndirectSpecularSettings* IndirectSpecularSettings = nullptr;
};
