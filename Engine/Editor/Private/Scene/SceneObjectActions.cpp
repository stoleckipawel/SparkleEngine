#include "PCH.h"
#include "Scene/SceneObjectActions.h"

#include "World/GameWorld.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/SceneMeshes.h"

namespace SceneObjectActions
{
	bool IsSelectionValid(const GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return gameWorld.IsEntityAlive(selection.entity) && gameWorld.GetCameras().IsCamera(selection.entity);
			case SceneObjectType::Sky:
				return true;
			case SceneObjectType::Light:
				return gameWorld.IsEntityAlive(selection.entity) && gameWorld.GetLighting().GetLight(selection.entity).has_value();
			case SceneObjectType::Mesh:
				return gameWorld.IsEntityAlive(selection.entity) && gameWorld.GetMeshes().GetMesh(selection.entity).IsValid();
			case SceneObjectType::None:
			default:
				return false;
		}
	}

	bool IsVisible(const GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return gameWorld.GetCameras().GetCamera(selection.entity).IsVisible();
			case SceneObjectType::Sky:
			{
				const std::optional<SceneSkyDesc> sky = gameWorld.GetSky().GetSky();
				return !sky || sky->enabled;
			}
			case SceneObjectType::Light:
				return gameWorld.GetLighting().IsLightVisible(selection.entity);
			case SceneObjectType::Mesh:
				return gameWorld.GetMeshes().GetMesh(selection.entity).IsVisible();
			case SceneObjectType::None:
			default:
				return true;
		}
	}

	void ToggleVisibility(GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
			{
				SceneCameraView camera = gameWorld.GetCameras().GetCamera(selection.entity);
				camera.SetVisible(!camera.IsVisible());
				break;
			}
			case SceneObjectType::Sky:
			{
				SceneSkyDesc sky = gameWorld.GetSky().GetSky().value_or(SceneSkyDesc{});
				sky.enabled = !sky.enabled;
				gameWorld.GetSky().SetSky(std::move(sky));
				break;
			}
			case SceneObjectType::Light:
				gameWorld.GetLighting().SetLightVisible(selection.entity, !gameWorld.GetLighting().IsLightVisible(selection.entity));
				break;
			case SceneObjectType::Mesh:
				if (SceneMeshView mesh = gameWorld.GetMeshes().GetMesh(selection.entity); mesh.IsValid())
				{
					mesh.SetVisible(!mesh.IsVisible());
				}
				break;
			case SceneObjectType::None:
			default:
				break;
		}
	}

	void ApplySelection(GameWorld& gameWorld, const SceneObjectSelection& selection) noexcept
	{
		if (selection.type == SceneObjectType::Camera)
		{
			gameWorld.GetCameras().SetActiveCamera(selection.entity);
		}
	}
}  // namespace SceneObjectActions
