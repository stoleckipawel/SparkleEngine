#include "PCH.h"
#include "Scene/SceneObjectActions.h"

#include "Scene/Camera/CameraComponent.h"
#include "Scene/GameScene.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"

namespace SceneObjectActions
{
	bool IsSelectionValid(const GameScene& gameScene, const SceneObjectSelection& selection) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return selection.index < gameScene.GetCameras().GetCameraCount();
			case SceneObjectType::Light:
				return selection.index < gameScene.GetLighting().GetLightCount();
			case SceneObjectType::Mesh:
				return selection.index < gameScene.GetMeshes().GetMeshCount();
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
				return gameScene.GetCameras().GetActiveCamera().GetCameraComponent().IsVisible();
			case SceneObjectType::Light:
				return gameScene.GetLighting().IsLightVisible(selection.index);
			case SceneObjectType::Mesh:
				if (selection.index >= gameScene.GetMeshes().GetMeshCount())
				{
					return true;
				}
				if (const MeshComponent* meshComponent = gameScene.GetMeshes().GetMeshComponent(selection.index))
				{
					return meshComponent->IsVisible();
				}
				return true;
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
				if (!gameScene.GetCameras().ApplyCamera(selection.index))
				{
					break;
				}
				CameraComponent& camera = gameScene.GetCameras().GetActiveCamera().GetCameraComponent();
				camera.SetVisible(!camera.IsVisible());
				break;
			}
			case SceneObjectType::Light:
				gameScene.GetLighting().SetLightVisible(selection.index, !gameScene.GetLighting().IsLightVisible(selection.index));
				break;
			case SceneObjectType::Mesh:
				if (selection.index < gameScene.GetMeshes().GetMeshCount())
				{
					if (MeshComponent* meshComponent = gameScene.GetMeshes().GetMeshComponent(selection.index))
					{
						meshComponent->SetVisible(!meshComponent->IsVisible());
					}
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
			gameScene.GetCameras().ApplyCamera(selection.index);
		}
	}
}  // namespace SceneObjectActions
