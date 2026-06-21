#pragma once

class RenderRayTracingScene;
struct RayTracedShadowSettings;
struct RTIndirectSpecularSettings;

struct RenderRayTracingPassServices
{
	RenderRayTracingScene* Scene = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
	const RTIndirectSpecularSettings* IndirectSpecularSettings = nullptr;
};
