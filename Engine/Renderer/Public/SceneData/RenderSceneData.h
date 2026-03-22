#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <vector>

struct SPARKLE_RENDERER_API RenderSceneData
{
	std::vector<DirectionalLight> directionalLights;
	std::vector<MeshDraw> meshDraws;
	std::vector<MaterialData> materials;
};
