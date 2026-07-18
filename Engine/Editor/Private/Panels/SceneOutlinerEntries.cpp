#include "PCH.h"
#include "Panels/SceneOutlinerEntries.h"

#include "Scene/SceneObjectPresentation.h"
namespace
{
	std::string BuildMeshLabel(std::size_t meshIndex)
	{
		return "Mesh " + std::to_string(meshIndex + 1);
	}
}  // namespace

namespace SceneOutlinerEntries
{
	std::vector<SceneOutlinerEntry> BuildCameraEntries(const WorldReadView& readView)
	{
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(readView.GetCameras().size());

		for (std::size_t cameraIndex = 0; cameraIndex < readView.GetCameras().size(); ++cameraIndex)
		{
			const WorldCameraReadData& camera = readView.GetCameras()[cameraIndex];
			entries.push_back(
			    SceneOutlinerEntry{
			        camera.Name.empty() ? "Camera " + std::to_string(cameraIndex + 1) : camera.Name,
			        "Camera",
			        SceneObjectSelection::Camera(camera.Entity)});
		}

		return entries;
	}

	std::vector<SceneOutlinerEntry> BuildLightEntries(const WorldReadView& readView)
	{
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(readView.GetLights().size());

		for (std::size_t lightIndex = 0; lightIndex < readView.GetLights().size(); ++lightIndex)
		{
			const WorldLightReadData& light = readView.GetLights()[lightIndex];
			entries.push_back(
			    SceneOutlinerEntry{
			        SceneObjectPresentation::BuildLightLabel(light.Description, lightIndex),
			        SceneObjectPresentation::GetLightTypeLabel(light.Description.GetKind()),
			        SceneObjectSelection::Light(light.Entity)});
		}

		return entries;
	}

	std::vector<SceneOutlinerEntry> BuildSkyEntries(const WorldReadView& readView)
	{
		return {SceneOutlinerEntry{
		    readView.GetSkyEnvironment().has_value() ? "Sky" : "Sky (Engine Default)",
		    "Sky",
		    SceneObjectSelection::Sky()}};
	}

	std::vector<SceneOutlinerEntry> BuildMeshEntries(const WorldReadView& readView)
	{
		const std::size_t meshCount = readView.GetMeshes().size();
		std::vector<SceneOutlinerEntry> entries;
		entries.reserve(meshCount);

		for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
		{
			entries.push_back(
			    SceneOutlinerEntry{
			        BuildMeshLabel(meshIndex),
			        "Static Mesh",
			        SceneObjectSelection::Mesh(readView.GetMeshes()[meshIndex].Entity)});
		}

		return entries;
	}
}  // namespace SceneOutlinerEntries
