#pragma once

class RenderRayTracingScene;
struct RayTracedShadowSettings;
struct IndirectSpecularSettings;

struct RenderRayTracingPassServices
{
	RenderRayTracingScene* Scene = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
	const IndirectSpecularSettings* IndirectSpecularSettings = nullptr;
};
