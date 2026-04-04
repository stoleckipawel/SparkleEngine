#include "PCH.h"

#include "RenderSceneDataBuilder.h"

#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/Mesh.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Caching/MaterialCacheUtils.h"

#include <cstddef>

RenderSceneDataBuilder::RenderSceneDataBuilder(MaterialCacheManager& materialCache, GPUMeshCache& gpuMeshCache) noexcept :
    m_materialCache(&materialCache), m_gpuMeshCache(&gpuMeshCache)
{
}

RenderSceneData RenderSceneDataBuilder::Build(const RenderSceneSnapshot& sceneSnapshot)
{
	RenderSceneData sceneData = {};

	if (!m_materialCache)
	{
		LOG_FATAL("RenderSceneDataBuilder::Build: material cache manager is unavailable.");
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
	sceneData.meshDraws.reserve(sceneSnapshot.meshes.meshComponents.size());

	for (const MeshComponent* meshComponent : sceneSnapshot.meshes.meshComponents)
	{
		if (meshComponent == nullptr)
		{
			continue;
		}

		const Mesh* mesh = meshComponent->GetMesh();
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
		DirectX::XMStoreFloat4x4(&draw.worldMatrix, meshComponent->GetWorldMatrix());
		DirectX::XMStoreFloat3x4(&draw.worldInvTranspose, meshComponent->GetWorldInverseTransposeMatrix());
		draw.materialSlot = MaterialCacheUtils::ResolveMaterialSlot(meshComponent->GetMaterialHandle(), sceneData.materials.size());
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
		sceneData.directionalLights.push_back(renderLight);
	}
}
