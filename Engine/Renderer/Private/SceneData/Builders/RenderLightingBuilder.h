#pragma once

#include "SceneData/Lifecycle/RenderSceneSnapshot.h"

struct RenderSceneData;

namespace RenderLightingBuilder
{
	void Build(const LightingSnapshot& lightingSnapshot, RenderSceneData& sceneData) noexcept;
}
