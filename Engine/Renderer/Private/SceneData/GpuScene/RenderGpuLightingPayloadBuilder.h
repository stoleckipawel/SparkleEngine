#pragma once

#include "SceneData/GpuScene/RenderGpuScenePayloads.h"

struct PreparedRenderScene;

class RenderGpuLightingPayloadBuilder final
{
public:
	static void Build(const PreparedRenderScene& preparedScene, RenderGpuLightingPayloads& payloads);
};
