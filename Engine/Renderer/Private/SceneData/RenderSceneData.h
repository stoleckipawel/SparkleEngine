#pragma once

#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/PointLight.h"
#include "Renderer/Public/SceneData/RectLight.h"
#include "Renderer/Public/SceneData/SpotLight.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderLightCollection.h"
#include "SceneData/RenderMeshWorkloadSummary.h"
#include "SceneData/RenderSkyData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Rendering/RenderObjectId.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"

#include <DirectXMath.h>

#include <vector>

struct ResolvedMaterialTextureTable final
{
	RhiDescriptorTableBinding Binding = {};
	std::uint32_t DescriptorCount = 0u;
	std::uint64_t Generation = 0u;

	constexpr explicit operator bool() const noexcept
	{
		return static_cast<bool>(Binding) && DescriptorCount != 0u && Generation != 0u;
	}
};

struct RenderSceneData
{
	std::uint64_t structuralRevision = 0;
	std::uint64_t materialRevision = 0;
	RenderLightCollection<DirectionalLight> directionalLights;
	RenderLightCollection<PointLight> pointLights;
	RenderLightCollection<SpotLight> spotLights;
	RenderLightCollection<RectLight> rectLights;
	RenderSkyData sky;
	std::vector<MeshDraw> meshInstances;
	std::vector<MeshInstanceBatch> meshInstanceBatches;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
	std::vector<DirectX::XMFLOAT4X4> previousJointMatrices;
	RenderMeshWorkloadSummary meshWorkload;
	std::vector<MaterialData> materials;
	ResolvedMaterialTextureTable materialTextureTable = {};
};
