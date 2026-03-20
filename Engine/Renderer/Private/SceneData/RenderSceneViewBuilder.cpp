#include "PCH.h"

#include "RenderSceneViewBuilder.h"

#include "RenderConfig.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/RenderSceneView.h"
#include "Scene/GameScene.h"
#include "Scene/Mesh.h"
#include "SceneData/MaterialCacheManager.h"
#include "SceneData/MaterialCacheUtils.h"

#include <algorithm>
#include <cstddef>

namespace
{
	std::size_t GetUploadedDirectionalLightCount(const RenderSceneView& renderSceneView) noexcept
	{
		return std::min(renderSceneView.directionalLights.size(), RenderConfig::Lights::MaxDirectionalLights);
	}
}  // namespace

RenderSceneViewBuilder::RenderSceneViewBuilder(MaterialCacheManager& materialCache) noexcept : m_materialCache(&materialCache) {}

RenderSceneView RenderSceneViewBuilder::BuildViewport(const GameScene& gameScene, const RenderSceneViewportDesc& viewportDesc)
{
	RenderSceneView renderSceneView = {};
	renderSceneView.width = viewportDesc.width;
	renderSceneView.height = viewportDesc.height;
	renderSceneView.camera = viewportDesc.camera;

	if (!m_materialCache)
	{
		LOG_FATAL("RenderSceneViewBuilder::BuildViewport: material cache manager is unavailable.");
		return renderSceneView;
	}

	if (viewportDesc.camera == nullptr)
	{
		LOG_FATAL("RenderSceneViewBuilder::BuildViewport: viewport camera is unavailable.");
		return renderSceneView;
	}

	m_materialCache->PopulateSceneMaterials(gameScene, renderSceneView);
	BuildMeshDraws(gameScene, renderSceneView);
	BuildLighting(gameScene, renderSceneView);

	return renderSceneView;
}

void RenderSceneViewBuilder::PopulatePerViewLightingData(const RenderSceneView& renderSceneView, PerViewConstantBufferData& perViewData)
    const noexcept
{
	perViewData.ViewLighting = {};
	const std::size_t directionalLightCount = GetUploadedDirectionalLightCount(renderSceneView);
	perViewData.ViewLighting.DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount);

	for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		perViewData.ViewLighting.DirectionalLights[lightIndex].Direction = renderSceneView.directionalLights[lightIndex].direction;
		perViewData.ViewLighting.DirectionalLights[lightIndex].Intensity = renderSceneView.directionalLights[lightIndex].intensity;
		perViewData.ViewLighting.DirectionalLights[lightIndex].Color = renderSceneView.directionalLights[lightIndex].color;
	}
}

void RenderSceneViewBuilder::BuildMeshDraws(const GameScene& gameScene, RenderSceneView& renderSceneView) const
{
	if (!gameScene.HasMeshes())
	{
		return;
	}

	const auto& meshes = gameScene.GetMeshes();
	renderSceneView.meshDraws.reserve(meshes.size());

	for (const auto& mesh : meshes)
	{
		MeshDraw draw = {};
		DirectX::XMStoreFloat4x4(&draw.worldMatrix, mesh->GetWorldMatrix());
		DirectX::XMStoreFloat3x4(&draw.worldInvTranspose, mesh->GetWorldInverseTransposeMatrix());
		draw.materialId = MaterialCacheUtils::ResolveMaterialId(mesh->GetMaterialId(), renderSceneView.materials.size());
		draw.meshPtr = mesh.get();
		renderSceneView.meshDraws.push_back(draw);
	}
}

void RenderSceneViewBuilder::BuildLighting(const GameScene& gameScene, RenderSceneView& renderSceneView) const noexcept
{
	const auto& lightingState = gameScene.GetLightingState();
	renderSceneView.directionalLights.clear();
	renderSceneView.directionalLights.reserve(lightingState.GetDirectionalLightCount());

	for (std::size_t lightIndex = 0; lightIndex < lightingState.GetDirectionalLightCount(); ++lightIndex)
	{
		const DirectionalLightDesc& directionalLight = lightingState.GetDirectionalLight(lightIndex);
		DirectionalLight renderLight = {};
		renderLight.direction = directionalLight.direction;
		renderLight.intensity = directionalLight.intensity;
		renderLight.color = directionalLight.color;
		renderSceneView.directionalLights.push_back(renderLight);
	}
}