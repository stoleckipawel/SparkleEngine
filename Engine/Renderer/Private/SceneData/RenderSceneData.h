#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <DirectXMath.h>

#include <vector>

struct SPARKLE_RENDERER_API RenderSceneData
{
	std::vector<DirectionalLight> directionalLights;
	std::vector<MeshDraw> meshInstances;
	std::vector<MeshInstanceBatch> meshInstanceBatches;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
	std::vector<MaterialData> materials;
};
