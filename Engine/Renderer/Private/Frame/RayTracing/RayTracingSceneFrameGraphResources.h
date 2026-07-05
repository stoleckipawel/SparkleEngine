#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

struct RayTracingSceneFrameGraphResources final
{
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();

	bool HasSceneTlas() const noexcept;
};
