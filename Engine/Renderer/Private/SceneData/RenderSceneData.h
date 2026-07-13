#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/PointLight.h"
#include "Renderer/Public/SceneData/RectLight.h"
#include "Renderer/Public/SceneData/SpotLight.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderMeshWorkloadSummary.h"
#include "SceneData/RenderSkyData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <DirectXMath.h>

#include <vector>

class RenderBindingSet;

struct SPARKLE_RENDERER_API RenderSceneData
{
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	std::vector<SpotLight> spotLights;
	std::vector<RectLight> rectLights;
	RenderSkyData sky;
	std::vector<MeshDraw> meshInstances;
	std::vector<MeshInstanceBatch> meshInstanceBatches;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
	std::vector<DirectX::XMFLOAT4X4> previousJointMatrices;
	RenderMeshWorkloadSummary meshWorkload;
	std::vector<MaterialData> materials;
	const RenderBindingSet* materialTextureTable = nullptr;
	std::uint32_t materialTextureTableDescriptorCount = 0u;
	bool materialTextureTableValid = false;
	const char* materialTextureTableStatusReason = "not-built";
};
