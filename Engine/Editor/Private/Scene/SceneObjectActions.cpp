#include "PCH.h"
#include "Scene/SceneObjectActions.h"

#include "Scene/GameScene.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/SceneMeshes.h"

namespace SceneObjectActions
{
	bool IsSelectionValid(const GameScene& gameScene, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return gameScene.IsEntityAlive(selection.entity) && gameScene.GetCameras().IsCamera(selection.entity);
			case SceneObjectType::Sky:
				return true;
			case SceneObjectType::Light:
				return gameScene.IsEntityAlive(selection.entity) && gameScene.GetLighting().GetLight(selection.entity).has_value();
			case SceneObjectType::Mesh:
				return gameScene.IsEntityAlive(selection.entity) && gameScene.GetMeshes().GetMesh(selection.entity).IsValid();
			case SceneObjectType::None:
			default:
				return false;
		}
	}

	bool IsVisible(const GameScene& gameScene, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return gameScene.GetCameras().GetCamera(selection.entity).IsVisible();
			case SceneObjectType::Sky:
			{
				const SceneSkyDesc* sky = gameScene.GetSky().GetSky();
				return sky == nullptr || sky->enabled;
			}
			case SceneObjectType::Light:
				return gameScene.GetLighting().IsLightVisible(selection.entity);
			case SceneObjectType::Mesh:
				return gameScene.GetMeshes().GetMesh(selection.entity).IsVisible();
			case SceneObjectType::None:
			default:
				return true;
		}
	}

	void ToggleVisibility(GameScene& gameScene, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
			{
				SceneCameraView camera = gameScene.GetCameras().GetCamera(selection.entity);
				camera.SetVisible(!camera.IsVisible());
				break;
			}
			case SceneObjectType::Sky:
			{
				SceneSkyDesc sky = gameScene.GetSky().GetSky() != nullptr ? *gameScene.GetSky().GetSky() : SceneSkyDesc{};
				sky.enabled = !sky.enabled;
				gameScene.GetSky().SetSky(std::move(sky));
				break;
			}
			case SceneObjectType::Light:
				gameScene.GetLighting().SetLightVisible(selection.entity, !gameScene.GetLighting().IsLightVisible(selection.entity));
				break;
			case SceneObjectType::Mesh:
				if (SceneMeshView mesh = gameScene.GetMeshes().GetMesh(selection.entity); mesh.IsValid())
				{
					mesh.SetVisible(!mesh.IsVisible());
				}
				break;
			case SceneObjectType::None:
			default:
				break;
		}
	}

	void ApplySelection(GameScene& gameScene, const SceneObjectSelection& selection) noexcept
	{
		if (selection.type == SceneObjectType::Camera)
		{
			gameScene.GetCameras().SetActiveCamera(selection.entity);
		}
	}
}  // namespace SceneObjectActions
