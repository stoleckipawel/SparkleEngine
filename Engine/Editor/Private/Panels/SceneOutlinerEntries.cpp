#include "PCH.h"
#include "Panels/SceneOutlinerEntries.h"

#include "Scene/SceneObjectPresentation.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Lighting/SceneLightDesc.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/SceneMeshes.h"

namespace
{
	std::string BuildMeshLabel(std::size_t meshIndex)
	{
		return "Mesh " + std::to_string(meshIndex + 1);
	}
}  // namespace

namespace SceneOutlinerEntries
{
	std::vector<SceneOutlinerEntry> BuildCameraEntries(const GameScene& gameScene)
	{
		const std::vector<SceneCameraEntry>& sceneCameras = gameScene.GetCameras().GetCameraEntries();
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(sceneCameras.size());

		for (std::size_t cameraIndex = 0; cameraIndex < sceneCameras.size(); ++cameraIndex)
		{
			const SceneCameraEntry& camera = sceneCameras[cameraIndex];
			entries.push_back(SceneOutlinerEntry{
			    camera.name.empty() ? "Camera " + std::to_string(cameraIndex + 1) : camera.name,
			    "Camera",
			    SceneObjectSelection::Camera(cameraIndex)});
		}

		return entries;
	}

	std::vector<SceneOutlinerEntry> BuildLightEntries(const GameScene& gameScene)
	{
		const std::vector<SceneLightDesc>& lights = gameScene.GetLighting().GetLights();
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(lights.size());

		for (std::size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
		{
			const SceneLightDesc& light = lights[lightIndex];
			entries.push_back(SceneOutlinerEntry{
			    SceneObjectPresentation::BuildLightLabel(light, lightIndex),
			    SceneObjectPresentation::GetLightTypeLabel(light.GetKind()),
			    SceneObjectSelection::Light(lightIndex)});
		}

		return entries;
	}

	std::vector<SceneOutlinerEntry> BuildMeshEntries(const GameScene& gameScene)
	{
		const std::size_t meshCount = gameScene.GetMeshes().GetMeshCount();
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(meshCount);

		for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
		{
			entries.push_back(SceneOutlinerEntry{BuildMeshLabel(meshIndex), "Static Mesh", SceneObjectSelection::Mesh(meshIndex)});
		}

		return entries;
	}
}  // namespace SceneOutlinerEntries
