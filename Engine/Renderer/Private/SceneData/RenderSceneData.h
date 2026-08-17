#pragma once

#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/PointLight.h"
#include "Renderer/Public/SceneData/RectLight.h"
#include "Renderer/Public/SceneData/SpotLight.h"
#include "SceneData/MaterialData.h"
#include "SceneData/MeshInstanceBatch.h"
#include "SceneData/RenderLightCollection.h"
#include "SceneData/RenderMeshWorldBounds.h"
#include "SceneData/RenderMeshWorkloadSummary.h"
#include "SceneData/RenderRayTracingWorkPlan.h"
#include "SceneData/RenderSkyData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Rendering/RenderObjectId.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"

#include <DirectXMath.h>

#include <span>
#include <vector>

struct ResolvedMaterialTextureTable final
{
	RhiDescriptorTableBinding Binding = {};
	std::uint32_t DescriptorCount = 0u;
	std::uint64_t Generation = 0u;

	explicit operator bool() const noexcept;
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
	std::vector<RenderMeshWorldBounds> meshWorldBounds;
	std::vector<std::uint32_t> rasterMeshInstanceIndices;
	std::vector<MeshInstanceBatch> meshInstanceBatches;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
	std::vector<DirectX::XMFLOAT4X4> previousJointMatrices;
	std::vector<float> morphWeights;
	std::vector<float> previousMorphWeights;
	RenderMeshWorkloadSummary meshWorkload;
	RenderRayTracingWorkPlan rayTracingWork;
	std::span<const MaterialData> materials;
	ResolvedMaterialTextureTable materialTextureTable = {};

	void ResetForReuse() noexcept;
};
