#include "PCH.h"
#include "Panels/SceneOutlinerEntries.h"

#include "Scene/SceneObjectPresentation.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Lighting/SceneLightDesc.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/SceneMeshes.h"

#include <optional>

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
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(gameScene.GetCameras().GetCameraCount());

		for (std::size_t cameraIndex = 0; cameraIndex < gameScene.GetCameras().GetCameraCount(); ++cameraIndex)
		{
			const SceneCameraEntry camera = gameScene.GetCameras().GetCameraEntry(cameraIndex);
			entries.push_back(
			    SceneOutlinerEntry{
			        camera.name.empty() ? "Camera " + std::to_string(cameraIndex + 1) : camera.name,
			        "Camera",
			        SceneObjectSelection::Camera(gameScene.GetCameras().GetCameraEntity(cameraIndex))});
		}

		return entries;
	}

	std::vector<SceneOutlinerEntry> BuildLightEntries(const GameScene& gameScene)
	{
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(gameScene.GetLighting().GetLightCount());

		for (std::size_t lightIndex = 0; lightIndex < gameScene.GetLighting().GetLightCount(); ++lightIndex)
		{
			const std::optional<SceneLightDesc> light = gameScene.GetLighting().GetLight(lightIndex);
			if (!light)
			{
				continue;
			}
			entries.push_back(
			    SceneOutlinerEntry{
			        SceneObjectPresentation::BuildLightLabel(*light, lightIndex),
			        SceneObjectPresentation::GetLightTypeLabel(light->GetKind()),
			        SceneObjectSelection::Light(gameScene.GetLighting().GetLightEntity(lightIndex))});
		}

		return entries;
	}

	std::vector<SceneOutlinerEntry> BuildSkyEntries(const GameScene& gameScene)
	{
		return {SceneOutlinerEntry{gameScene.GetSky().HasSky() ? "Sky" : "Sky (Engine Default)", "Sky", SceneObjectSelection::Sky()}};
	}

	std::vector<SceneOutlinerEntry> BuildMeshEntries(const GameScene& gameScene)
	{
		const std::size_t meshCount = gameScene.GetMeshes().GetMeshCount();
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(meshCount);

		for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
		{
			entries.push_back(
			    SceneOutlinerEntry{
			        BuildMeshLabel(meshIndex),
			        "Static Mesh",
			        SceneObjectSelection::Mesh(gameScene.GetMeshes().GetMeshEntity(meshIndex))});
		}

		return entries;
	}
}  // namespace SceneOutlinerEntries
