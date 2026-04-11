#include "PCH.h"
#include "Panels/SceneInspectorPanel.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/SceneObjectSelection.h"
#include "Scene/Camera/CameraComponent.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/GameScene.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Lighting/DirectionalLightComponent.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Scene/Transform.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include <imgui.h>

SceneInspectorPanel::SceneInspectorPanel(GameScene& gameScene, SceneObjectSelection& selection, float widthPixels) noexcept :
    m_gameScene(&gameScene), m_selection(&selection), m_widthPixels(widthPixels)
{
}

std::string SceneInspectorPanel::BuildSelectionTitle() const
{
	if (m_selection == nullptr)
	{
		return "No Selection";
	}

	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			return "Scene Camera";
		case SceneObjectType::DirectionalLight:
			return "Directional Light " + std::to_string(m_selection->index + 1);
		case SceneObjectType::Mesh:
			return "Mesh " + std::to_string(m_selection->index + 1);
		case SceneObjectType::None:
		default:
			return "No Selection";
	}
}

const char* SceneInspectorPanel::BuildSelectionSubtitle() const noexcept
{
	if (m_selection == nullptr)
	{
		return "Object";
	}

	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			return "Camera";
		case SceneObjectType::DirectionalLight:
			return "Directional Light";
		case SceneObjectType::Mesh:
			return "Static Mesh";
		case SceneObjectType::None:
		default:
			return "Object";
	}
}

void SceneInspectorPanel::BuildSelectionHeader() noexcept
{
	UiUtil::BeginSectionCard("Selection");
	ImGui::TextUnformatted(BuildSelectionTitle().c_str());
	ImGui::TextDisabled("%s", BuildSelectionSubtitle());
	UiUtil::EndSectionCard();
	ImGui::Dummy(ImVec2(0.0f, 6.0f));
}

void SceneInspectorPanel::SetWidth(float widthPixels) noexcept
{
	m_widthPixels = widthPixels;
}

void SceneInspectorPanel::SetTopInset(float topInsetPixels) noexcept
{
	m_topInsetPixels = topInsetPixels;
}

void SceneInspectorPanel::ClampCameraUiValues(float& pitchDegrees, float& fovYDegrees, float& moveSpeed) noexcept
{
	pitchDegrees = std::clamp(pitchDegrees, -89.0f, 89.0f);
	fovYDegrees = std::clamp(fovYDegrees, 1.0f, 179.0f);
	moveSpeed = std::clamp(moveSpeed, 0.0001f, 10.0f);
}

void SceneInspectorPanel::ClampLightingUiValues(DirectX::XMFLOAT3& color, float& intensity) noexcept
{
	color.x = std::clamp(color.x, 0.0f, 1.0f);
	color.y = std::clamp(color.y, 0.0f, 1.0f);
	color.z = std::clamp(color.z, 0.0f, 1.0f);
	intensity = (std::max) (0.0f, intensity);
}

void SceneInspectorPanel::BuildEmptyState() noexcept
{
	UiUtil::BeginSectionCard("No Selection");
	ImGui::TextDisabled("Select an object from the scene outliner to inspect its properties.");
	UiUtil::EndSectionCard();
}

void SceneInspectorPanel::BuildCameraInspector() noexcept
{
	SceneCamera& sceneCamera = m_gameScene->GetSceneCamera();
	CameraComponent& cameraComponent = sceneCamera.GetCameraComponent();
	CameraMovementSettings settings = sceneCamera.GetSettings();
	const DirectX::XMFLOAT3 rotationEuler = cameraComponent.GetTransform().GetRotationEuler();

	UiUtil::BeginSectionCard("Camera");
	DirectX::XMFLOAT3 position = cameraComponent.GetTransform().GetTranslation();
	float positionValues[3] = {position.x, position.y, position.z};
	if (UiUtil::EditFloat3SliderWithInput("Position", positionValues, kPositionSliderMin, kPositionSliderMax, "%.2f", "%.3f"))
	{
		cameraComponent.SetPosition({positionValues[0], positionValues[1], positionValues[2]});
	}

	float yawDegrees = MathUtils::RadiansToDegrees(rotationEuler.y);
	if (UiUtil::EditFloatSliderWithInput("Yaw", yawDegrees, kYawSliderMin, kYawSliderMax, "%.1f deg", "%.2f"))
	{
		cameraComponent.SetYawPitch(MathUtils::DegreesToRadians(yawDegrees), rotationEuler.x);
	}

	float pitchDegrees = MathUtils::RadiansToDegrees(rotationEuler.x);
	if (UiUtil::EditFloatSliderWithInput("Pitch", pitchDegrees, -89.0f, 89.0f, "%.1f deg", "%.2f"))
	{
		float dummyFov = 60.0f;
		float dummySpeed = 0.15f;
		ClampCameraUiValues(pitchDegrees, dummyFov, dummySpeed);
		cameraComponent.SetYawPitch(rotationEuler.y, MathUtils::DegreesToRadians(pitchDegrees));
	}

	float fovYDegrees = cameraComponent.GetFovYDegrees();
	if (UiUtil::EditFloatSliderWithInput("FOV", fovYDegrees, 1.0f, 179.0f, "%.1f deg", "%.1f"))
	{
		float dummyPitch = 0.0f;
		float dummySpeed = 0.0f;
		ClampCameraUiValues(dummyPitch, fovYDegrees, dummySpeed);
		cameraComponent.SetFovYDegrees(fovYDegrees);
	}

	float moveSpeed = settings.moveSpeed;
	if (UiUtil::EditFloatSliderWithInput("Speed", moveSpeed, 0.0001f, 10.0f, "%.4f", "%.4f"))
	{
		float dummyPitch = 0.0f;
		float dummyFov = 60.0f;
		ClampCameraUiValues(dummyPitch, dummyFov, moveSpeed);
		settings.moveSpeed = moveSpeed;
		sceneCamera.SetSettings(settings);
	}

	char buffer[64] = {};
	std::snprintf(buffer, sizeof(buffer), "%.3f", cameraComponent.GetNearZ());
	UiUtil::DrawKeyValueRow("Near", buffer);
	std::snprintf(buffer, sizeof(buffer), "%.3f", cameraComponent.GetFarZ());
	UiUtil::DrawKeyValueRow("Far", buffer);
	std::snprintf(buffer, sizeof(buffer), "%.3f", cameraComponent.GetAspectRatio());
	UiUtil::DrawKeyValueRow("Aspect", buffer);
	UiUtil::EndSectionCard();
}

void SceneInspectorPanel::BuildDirectionalLightInspector(std::size_t lightIndex) noexcept
{
	if (lightIndex >= m_gameScene->GetLighting().GetDirectionalLightCount())
	{
		BuildEmptyState();
		return;
	}

	DirectionalLightComponent& light = m_gameScene->GetLighting().GetDirectionalLightComponent(lightIndex);
	DirectionalLightDesc lightDesc = light.GetDesc();
	const std::string cardTitle = "Directional Light " + std::to_string(lightIndex + 1);

	UiUtil::BeginSectionCard(cardTitle.c_str());
	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	if (UiUtil::EditFloat3SliderWithInput("Direction", directionValues, kDirectionSliderMin, kDirectionSliderMax, "%.2f", "%.3f"))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
		light.ApplyDesc(lightDesc);
	}

	float intensity = lightDesc.intensity;
	if (UiUtil::EditFloatSliderWithInput("Intensity", intensity, kIntensitySliderMin, kIntensitySliderMax, "%.2f", "%.3f"))
	{
		DirectX::XMFLOAT3 dummyColor = {1.0f, 1.0f, 1.0f};
		ClampLightingUiValues(dummyColor, intensity);
		lightDesc.intensity = intensity;
		light.ApplyDesc(lightDesc);
	}

	float colorValues[3] = {lightDesc.color.x, lightDesc.color.y, lightDesc.color.z};
	if (UiUtil::EditColor3("Color", colorValues))
	{
		DirectX::XMFLOAT3 clampedColor = {colorValues[0], colorValues[1], colorValues[2]};
		float dummyIntensity = 1.0f;
		ClampLightingUiValues(clampedColor, dummyIntensity);
		lightDesc.color = clampedColor;
		light.ApplyDesc(lightDesc);
	}
	UiUtil::EndSectionCard();
}

void SceneInspectorPanel::BuildEditableMeshTransform(MeshComponent& meshComponent) noexcept
{
	Transform& transform = meshComponent.GetTransform();
	DirectX::XMFLOAT3 translation = transform.GetTranslation();
	float translationValues[3] = {translation.x, translation.y, translation.z};
	if (UiUtil::EditFloat3SliderWithInput("Position", translationValues, kPositionSliderMin, kPositionSliderMax, "%.2f", "%.3f"))
	{
		transform.SetTranslation({translationValues[0], translationValues[1], translationValues[2]});
	}

	DirectX::XMFLOAT3 rotationEuler = transform.GetRotationEuler();
	float rotationValues[3] = {
	    MathUtils::RadiansToDegrees(rotationEuler.x),
	    MathUtils::RadiansToDegrees(rotationEuler.y),
	    MathUtils::RadiansToDegrees(rotationEuler.z)};
	if (UiUtil::EditFloat3SliderWithInput("Rotation", rotationValues, -360.0f, 360.0f, "%.1f", "%.2f"))
	{
		transform.SetRotationEuler(
		    {MathUtils::DegreesToRadians(rotationValues[0]),
		     MathUtils::DegreesToRadians(rotationValues[1]),
		     MathUtils::DegreesToRadians(rotationValues[2])});
	}

	DirectX::XMFLOAT3 scale = transform.GetScale();
	float scaleValues[3] = {scale.x, scale.y, scale.z};
	if (UiUtil::EditFloat3SliderWithInput("Scale", scaleValues, kScaleSliderMin, kScaleSliderMax, "%.2f", "%.3f"))
	{
		transform.SetScale({scaleValues[0], scaleValues[1], scaleValues[2]});
	}
}

void SceneInspectorPanel::BuildMeshInspector(std::size_t meshIndex) noexcept
{
	if (meshIndex >= m_gameScene->GetMeshes().GetMeshCount())
	{
		BuildEmptyState();
		return;
	}

	MeshComponent* meshComponent = m_gameScene->GetMeshes().GetMeshComponent(meshIndex);
	if (meshComponent == nullptr)
	{
		BuildEmptyState();
		return;
	}

	Mesh* mesh = meshComponent->GetMesh();
	if (mesh == nullptr)
	{
		BuildEmptyState();
		return;
	}

	const std::string cardTitle = "Mesh " + std::to_string(meshIndex + 1);
	UiUtil::BeginSectionCard(cardTitle.c_str());
	const bool isCookedMesh = dynamic_cast<const CookedMesh*>(mesh) != nullptr;
	UiUtil::DrawKeyValueRow("Type", isCookedMesh ? "Cooked" : "Procedural");

	char buffer[64] = {};
	const MaterialHandle materialHandle = meshComponent->GetMaterialHandle();
	std::snprintf(buffer, sizeof(buffer), "%u", materialHandle.IsValid() ? materialHandle.GetIndex() : 0u);
	UiUtil::DrawKeyValueRow("Material", buffer);

	const MeshData& meshData = mesh->GetMeshData();
	std::snprintf(buffer, sizeof(buffer), "%u", meshData.GetVertexCount());
	UiUtil::DrawKeyValueRow("Vertices", buffer);
	std::snprintf(buffer, sizeof(buffer), "%u", meshData.GetIndexCount());
	UiUtil::DrawKeyValueRow("Indices", buffer);

	BuildEditableMeshTransform(*meshComponent);
	UiUtil::EndSectionCard();
}

void SceneInspectorPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - m_widthPixels, m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, io.DisplaySize.y - m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.98f);

	ImGui::Begin(
	    "Inspector",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	UiUtil::DrawPanelHeader("Inspector", "Selection");

	if (m_gameScene == nullptr || m_selection == nullptr)
	{
		ImGui::TextDisabled("Scene inspector unavailable");
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(disableInteraction);
	BuildSelectionHeader();
	switch (m_selection->type)
	{
		case SceneObjectType::Camera:
			BuildCameraInspector();
			break;
		case SceneObjectType::DirectionalLight:
			BuildDirectionalLightInspector(m_selection->index);
			break;
		case SceneObjectType::Mesh:
			BuildMeshInspector(m_selection->index);
			break;
		case SceneObjectType::None:
		default:
			BuildEmptyState();
			break;
	}
	ImGui::EndDisabled();

	ImGui::End();
}