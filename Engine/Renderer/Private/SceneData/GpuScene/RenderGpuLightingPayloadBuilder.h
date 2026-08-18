#pragma once

#include "SceneData/GpuScene/RenderGpuScenePayloads.h"

struct RenderSceneData;

class RenderGpuLightingPayloadBuilder final
{
public:
	static void Build(const RenderSceneData& sceneData, RenderGpuLightingPayloads& payloads);
};
