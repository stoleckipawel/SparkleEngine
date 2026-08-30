#include "PCH.h"
#include "Scene/SceneObjectPresentation.h"

#include "Scene/SceneObjectSelection.h"
#include "Scene/Lighting/SceneLightDesc.h"

namespace SceneObjectPresentation
{
	UiUtil::EditorIcon GetLightIcon(SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return UiUtil::EditorIcon::DirectionalLight;
			case SceneLightKind::Point:
				return UiUtil::EditorIcon::PointLight;
			case SceneLightKind::Spot:
				return UiUtil::EditorIcon::SpotLight;
			case SceneLightKind::Rect:
				return UiUtil::EditorIcon::Light;
			case SceneLightKind::Unknown:
			default:
				return UiUtil::EditorIcon::Light;
		}
	}

	const char* GetLightTypeLabel(SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return "Directional Light";
			case SceneLightKind::Point:
				return "Point Light";
			case SceneLightKind::Spot:
				return "Spot Light";
			case SceneLightKind::Rect:
				return "Rect Light";
			case SceneLightKind::Unknown:
			default:
				return "Light";
		}
	}

	std::string BuildLightLabel(const SceneLightDesc& light, std::size_t lightIndex)
	{
		if (!light.common.name.empty())
		{
			return light.common.name;
		}

		return std::string(GetLightTypeLabel(light.GetKind())) + " " + std::to_string(lightIndex + 1);
	}

	UiUtil::EditorIcon BuildSelectionIcon(const SceneObjectSelection& selection, SceneLightKind lightKind) noexcept
	{
		switch (selection.type)
		{
			case SceneObjectType::Camera:
				return UiUtil::EditorIcon::Camera;
			case SceneObjectType::Sky:
				return UiUtil::EditorIcon::ViewLit;
			case SceneObjectType::Light:
				return GetLightIcon(lightKind);
			case SceneObjectType::Mesh:
				return UiUtil::EditorIcon::StaticMesh;
			case SceneObjectType::None:
			default:
				return UiUtil::EditorIcon::None;
		}
	}

	UiUtil::EditorIcon BuildSelectionIcon(const SceneObjectSelection* selection, SceneLightKind lightKind) noexcept
	{
		return selection != nullptr ? BuildSelectionIcon(*selection, lightKind) : UiUtil::EditorIcon::None;
	}
}
