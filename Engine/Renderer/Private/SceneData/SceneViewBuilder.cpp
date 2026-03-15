#include "PCH.h"

#include "SceneViewBuilder.h"

#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/SceneData/SceneView.h"
#include "SceneData/MaterialCacheManager.h"
#include "SceneData/MaterialCacheUtils.h"
#include "SceneData/RenderSceneSnapshot.h"

SceneViewBuilder::SceneViewBuilder(MaterialCacheManager& materialCache) noexcept : m_materialCache(&materialCache) {}

SceneView SceneViewBuilder::Build(
    const RenderSceneSnapshot& sceneSnapshot,
    const RenderCamera& renderCamera,
    std::uint32_t width,
    std::uint32_t height)
{
	SceneView view = {};
	view.width = width;
	view.height = height;
	view.camera = &renderCamera;

	if (!m_materialCache)
	{
		LOG_FATAL("SceneViewBuilder::Build: material cache manager is unavailable.");
		return view;
	}

	m_materialCache->PopulateSceneMaterials(sceneSnapshot, view);
	BuildMeshDraws(sceneSnapshot, view);

	return view;
}

void SceneViewBuilder::BuildMeshDraws(const RenderSceneSnapshot& sceneSnapshot, SceneView& view) const
{
	if (!sceneSnapshot.HasMeshes())
	{
		return;
	}

	view.meshDraws.reserve(sceneSnapshot.meshes.size());

	for (const RenderSceneMeshRecord& meshRecord : sceneSnapshot.meshes)
	{
		MeshDraw draw = {};
		draw.worldMatrix = meshRecord.worldMatrix;
		draw.worldInvTranspose = meshRecord.worldInvTranspose;
		draw.materialId = MaterialCacheUtils::ResolveMaterialId(meshRecord.materialId, view.materials.size());
		draw.meshPtr = meshRecord.mesh;
		view.meshDraws.push_back(draw);
	}
}