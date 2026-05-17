#include "PCH.h"

#include "RenderSceneDataBuilder.h"

#include "Meshes/GPUMeshCache.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "SceneData/RenderSceneData.h"
#include "Scene/Meshes/Mesh.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Caching/MaterialCacheUtils.h"

#include <cstddef>

static const auto g_renderSceneDataBuilderLogger = Logging::GetOrCreateLogger("Renderer.SceneData");

RenderSceneDataBuilder::RenderSceneDataBuilder(MaterialCacheManager& materialCache, GPUMeshCache& gpuMeshCache) noexcept :
    m_materialCache(&materialCache), m_gpuMeshCache(&gpuMeshCache)
{
}

RenderSceneData RenderSceneDataBuilder::Build(const RenderSceneSnapshot& sceneSnapshot)
{
	RenderSceneData sceneData = {};

	if (!m_materialCache)
	{
		Diagnostics::Fail(
		    g_renderSceneDataBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "RenderSceneDataBuilder::Build: material cache manager is unavailable.");
		return sceneData;
	}

	BuildMaterials(sceneSnapshot, sceneData);
	BuildMeshDraws(sceneSnapshot, sceneData);
	BuildLighting(sceneSnapshot, sceneData);
	return sceneData;
}

void RenderSceneDataBuilder::BuildMaterials(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const
{
	if (m_materialCache == nullptr)
	{
		return;
	}

	m_materialCache->BuildMaterials(sceneSnapshot.materials, sceneData);
}

void RenderSceneDataBuilder::BuildMeshDraws(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const
{
	if (!sceneSnapshot.meshes.HasMeshes() || m_gpuMeshCache == nullptr)
	{
		return;
	}

	sceneData.meshDraws.clear();
	sceneData.meshDraws.reserve(sceneSnapshot.meshes.meshInstances.size());

	for (const MeshInstanceSnapshot& meshInstance : sceneSnapshot.meshes.meshInstances)
	{
		const Mesh* mesh = meshInstance.mesh;
		if (mesh == nullptr)
		{
			continue;
		}

		GPUMesh* gpuMesh = m_gpuMeshCache->GetOrUpload(*mesh);
		if (gpuMesh == nullptr || !gpuMesh->IsValid())
		{
			continue;
		}

		MeshDraw draw = {};
		draw.worldMatrix = meshInstance.worldMatrix;
		draw.worldInvTranspose = meshInstance.worldInvTranspose;
		draw.materialSlot = MaterialCacheUtils::ResolveMaterialSlot(meshInstance.materialHandle, sceneData.materials.size());
		draw.gpuMesh = gpuMesh;
		sceneData.meshDraws.push_back(draw);
	}
}

void RenderSceneDataBuilder::BuildLighting(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const noexcept
{
	const LightingSnapshot& lightingSnapshot = sceneSnapshot.lighting;
	sceneData.directionalLights.clear();
	sceneData.directionalLights.reserve(lightingSnapshot.directionalLightCount);

	for (std::size_t lightIndex = 0; lightIndex < lightingSnapshot.directionalLightCount; ++lightIndex)
	{
		const DirectionalLightDesc& light = lightingSnapshot.directionalLights[lightIndex];
		DirectionalLight renderLight = {};
		renderLight.direction = light.direction;
		renderLight.intensity = light.intensity;
		renderLight.color = light.color;
		renderLight.castShadow = light.castShadow;
		sceneData.directionalLights.push_back(renderLight);
	}
}
