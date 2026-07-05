#include "../../PCH.h"

#include "Frame/RayTracing/RayTracingSceneFrameGraphResources.h"

bool RayTracingSceneFrameGraphResources::HasSceneTlas() const noexcept
{
	return SceneTlas.IsValid();
}
