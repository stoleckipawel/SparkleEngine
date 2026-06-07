#include "PCH.h"
#include "Panels/SceneCameraInspector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/CameraComponent.h"
#include "Scene/Camera/SceneCamera.h"
#include "Util/UiUtil.h"

#include <cstdio>

void SceneCameraInspector::Build(GameScene& gameScene, const std::string& filterText) noexcept
{
	SceneCamera& sceneCamera = gameScene.GetCameras().GetActiveCamera();
	CameraComponent& cameraComponent = sceneCamera.GetCameraComponent();

	BuildTransformCategory(filterText, cameraComponent);
	BuildCameraCategory(filterText, cameraComponent);
	BuildMovementCategory(filterText, sceneCamera);
	BuildAdvancedParametersCategory(filterText, cameraComponent);
}

void SceneCameraInspector::BuildTransformCategory(const std::string& filterText, CameraComponent& cameraComponent) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Transform", "location rotation scale position"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Transform"))
	{
		return;
	}

	DirectX::XMFLOAT3 position = cameraComponent.GetTransform().GetTranslation();
	float positionValues[3] = {position.x, position.y, position.z};
	const float defaultPosition[3] = {0.0f, 0.0f, -4.0f};
	if (UiUtil::EditDetailsFloat3("Location", positionValues, 0.05f, kPositionSliderMin, kPositionSliderMax, "%.3f", defaultPosition))
	{
		cameraComponent.SetPosition({positionValues[0], positionValues[1], positionValues[2]});
	}

	DirectX::XMFLOAT3 rotationEuler = cameraComponent.GetTransform().GetRotationEuler();
	const DirectX::XMFLOAT3 rotationDegrees = MathUtils::RadiansToDegrees(rotationEuler);
	float rotationValues[3] = {rotationDegrees.x, rotationDegrees.y, rotationDegrees.z};
	const float defaultRotation[3] = {0.0f, 0.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f", defaultRotation))
	{
		cameraComponent.SetRotationEuler(
		    MathUtils::DegreesToRadians(DirectX::XMFLOAT3{rotationValues[0], rotationValues[1], rotationValues[2]}));
	}

	DirectX::XMFLOAT3 scale = cameraComponent.GetTransform().GetScale();
	float scaleValues[3] = {scale.x, scale.y, scale.z};
	const float defaultScale[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsFloat3("Scale", scaleValues, 0.01f, kScaleSliderMin, kScaleSliderMax, "%.3f", defaultScale))
	{
		cameraComponent.SetScale({scaleValues[0], scaleValues[1], scaleValues[2]});
	}

	UiUtil::EndDetailsCategory();
}

void SceneCameraInspector::BuildCameraCategory(const std::string& filterText, CameraComponent& cameraComponent) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Camera", "field of view near clip far clip aspect ratio"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Camera"))
	{
		return;
	}

	char buffer[64] = {};
	float fovYDegrees = cameraComponent.GetFovYDegrees();
	constexpr float kDefaultFovYDegrees = 60.0f;
	if (UiUtil::EditDetailsFloat("Field Of View", fovYDegrees, 0.1f, 1.0f, 179.0f, "%.1f", &kDefaultFovYDegrees))
	{
		cameraComponent.SetFovYDegrees(fovYDegrees);
	}

	float nearZ = cameraComponent.GetNearZ();
	constexpr float kDefaultNearZ = 0.1f;
	if (UiUtil::EditDetailsFloat("Near Clip", nearZ, 0.01f, 0.001f, cameraComponent.GetFarZ(), "%.3f", &kDefaultNearZ))
	{
		cameraComponent.SetNearFar(nearZ, cameraComponent.GetFarZ());
	}

	float farZ = cameraComponent.GetFarZ();
	constexpr float kDefaultFarZ = 1000.0f;
	if (UiUtil::EditDetailsFloat("Far Clip", farZ, 1.0f, cameraComponent.GetNearZ(), 100000.0f, "%.3f", &kDefaultFarZ))
	{
		cameraComponent.SetNearFar(cameraComponent.GetNearZ(), farZ);
	}

	std::snprintf(buffer, sizeof(buffer), "%.3f", cameraComponent.GetAspectRatio());
	UiUtil::DrawDetailsValueRow("Aspect Ratio", buffer);
	UiUtil::EndDetailsCategory();
}

void SceneCameraInspector::BuildMovementCategory(const std::string& filterText, SceneCamera& sceneCamera) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Movement", "move speed navigation"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Movement"))
	{
		return;
	}

	CameraMovementSettings settings = sceneCamera.GetSettings();
	float moveSpeed = settings.moveSpeed;
	constexpr float kDefaultMoveSpeed = 0.10f;
	if (UiUtil::EditDetailsFloat("Move Speed", moveSpeed, 0.01f, 0.0001f, 10.0f, "%.4f", &kDefaultMoveSpeed))
	{
		settings.moveSpeed = moveSpeed;
		sceneCamera.SetSettings(settings);
	}

	UiUtil::EndDetailsCategory();
}

void SceneCameraInspector::BuildAdvancedParametersCategory(const std::string& filterText, CameraComponent& cameraComponent) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Advanced", "visible visibility hidden"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Advanced", false))
	{
		return;
	}

	constexpr bool kDefaultVisible = true;
	bool visible = cameraComponent.IsVisible();
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &kDefaultVisible))
	{
		cameraComponent.SetVisible(visible);
	}

	UiUtil::EndDetailsCategory();
}
