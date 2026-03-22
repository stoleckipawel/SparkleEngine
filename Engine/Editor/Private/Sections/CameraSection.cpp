#include "PCH.h"
#include "Sections/CameraSection.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/Camera/GameCamera.h"
#include "Util/UiUtil.h"

#include <algorithm>

#include <imgui.h>

void CameraSection::ClampCameraUiValues(float& pitchDegrees, float& fovYDegrees, float& moveSpeed) noexcept
{
	pitchDegrees = std::clamp(pitchDegrees, -89.0f, 89.0f);
	fovYDegrees = std::clamp(fovYDegrees, 1.0f, 179.0f);
	moveSpeed = std::clamp(moveSpeed, 0.0001f, 10.0f);
}

CameraSection::CameraSection(SceneCamera& sceneCamera) noexcept : m_sceneCamera(&sceneCamera) {}

void CameraSection::BuildUI()
{
	if (m_sceneCamera == nullptr)
	{
		ImGui::TextDisabled("Camera controls unavailable");
		return;
	}

	GameCamera& gameCamera = m_sceneCamera->GetGameCamera();
	CameraMovementSettings settings = m_sceneCamera->GetSettings();
	const DirectX::XMFLOAT3 rotationEuler = gameCamera.GetTransform().GetRotationEuler();

	DirectX::XMFLOAT3 position = gameCamera.GetTransform().GetTranslation();
	float positionValues[3] = {position.x, position.y, position.z};
	ImGui::PushID("Position");
	if (UiUtil::EditFloat3SliderWithInput("Position", positionValues, kPositionSliderMin, kPositionSliderMax, "%.2f", "%.3f"))
	{
		gameCamera.SetPosition({positionValues[0], positionValues[1], positionValues[2]});
	}
	ImGui::PopID();

	float yawDegrees = MathUtils::RadiansToDegrees(rotationEuler.y);
	ImGui::PushID("Yaw");
	if (UiUtil::EditFloatSliderWithInput("Yaw", yawDegrees, kYawSliderMin, kYawSliderMax, "%.1f deg", "%.2f"))
	{
		gameCamera.SetYawPitch(MathUtils::DegreesToRadians(yawDegrees), rotationEuler.x);
	}
	ImGui::PopID();

	float pitchDegrees = MathUtils::RadiansToDegrees(rotationEuler.x);
	ImGui::PushID("Pitch");
	if (UiUtil::EditFloatSliderWithInput("Pitch", pitchDegrees, -89.0f, 89.0f, "%.1f deg", "%.2f"))
	{
		float dummyFov = 60.0f;
		float dummySpeed = 0.15f;
		ClampCameraUiValues(pitchDegrees, dummyFov, dummySpeed);
		gameCamera.SetYawPitch(rotationEuler.y, MathUtils::DegreesToRadians(pitchDegrees));
	}
	ImGui::PopID();

	float fovYDegrees = gameCamera.GetFovYDegrees();
	ImGui::PushID("FOV");
	if (UiUtil::EditFloatSliderWithInput("FOV", fovYDegrees, 1.0f, 179.0f, "%.1f deg", "%.1f"))
	{
		float dummyPitch = 0.0f;
		float dummySpeed = 0.0f;
		ClampCameraUiValues(dummyPitch, fovYDegrees, dummySpeed);
		gameCamera.SetFovYDegrees(fovYDegrees);
	}
	ImGui::PopID();

	float moveSpeed = settings.moveSpeed;
	ImGui::PushID("Speed");
	if (UiUtil::EditFloatSliderWithInput("Speed", moveSpeed, 0.0001f, 10.0f, "%.4f", "%.4f"))
	{
		float dummyPitch = 0.0f;
		float dummyFov = 60.0f;
		ClampCameraUiValues(dummyPitch, dummyFov, moveSpeed);
		settings.moveSpeed = moveSpeed;
		m_sceneCamera->SetSettings(settings);
	}
	ImGui::PopID();
}