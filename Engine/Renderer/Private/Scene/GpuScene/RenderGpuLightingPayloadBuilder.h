#pragma once

#include "Scene/GpuScene/RenderGpuScenePayloads.h"

struct PreparedRenderScene;

class RenderGpuLightingPayloadBuilder final
{
public:
	static void Build(const PreparedRenderScene& preparedScene, RenderGpuLightingPayloads& payloads);
};
