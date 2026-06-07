#pragma once

class RenderRayTracingScene;
struct RayTracedShadowSettings;

struct RenderRayTracingPassServices
{
	RenderRayTracingScene* Scene = nullptr;
	const RayTracedShadowSettings* ShadowSettings = nullptr;
};
