#include "PCH.h"
#include "Sections/CameraSection.h"

#include "Core/Public/Math/MathUtils.h"
#include "Runtime/Level/LevelManager.h"
#include "Scene/Camera/GameCameraController.h"
#include "Util/UiUtil.h"

#include <algorithm>

#include <imgui.h>

namespace
{
	constexpr float PositionSliderMin = -500.0f;
	constexpr float PositionSliderMax = 500.0f;
	constexpr float YawSliderMin = -360.0f;
	constexpr float YawSliderMax = 360.0f;

	void ClampCameraUiValues(float& pitchDegrees, float& fovYDegrees, float& moveSpeed)
	{
		pitchDegrees = std::clamp(pitchDegrees, -89.0f, 89.0f);
		fovYDegrees = std::clamp(fovYDegrees, 1.0f, 179.0f);
		moveSpeed = std::clamp(moveSpeed, 0.0001f, 10.0f);
	}
}  // namespace

CameraSection::CameraSection(LevelManager& levelManager) noexcept : m_levelManager(&levelManager) {}

void CameraSection::BuildUI()
{
	if (m_levelManager == nullptr)
	{
		ImGui::TextDisabled("Camera controls unavailable");
		return;
	}

	GameCameraController* gameCameraController = m_levelManager->GetGameCameraController();
	if (gameCameraController == nullptr)
	{
		ImGui::TextDisabled("Camera controller unavailable");
		return;
	}

	const bool hasActiveLevel = m_levelManager->HasActiveLevel();
	const ImGuiStyle& style = ImGui::GetStyle();

	DirectX::XMFLOAT3 position = gameCameraController->GetPosition();
	float positionValues[3] = {position.x, position.y, position.z};
	ImGui::PushID("Position");
	if (UiUtil::EditFloat3SliderWithInput("Position", positionValues, PositionSliderMin, PositionSliderMax, "%.2f", "%.3f"))
	{
		gameCameraController->SetPosition({positionValues[0], positionValues[1], positionValues[2]});
	}
	ImGui::PopID();

	float yawDegrees = MathUtils::RadiansToDegrees(gameCameraController->GetYaw());
	ImGui::PushID("Yaw");
	if (UiUtil::EditFloatSliderWithInput("Yaw", yawDegrees, YawSliderMin, YawSliderMax, "%.1f deg", "%.2f"))
	{
		gameCameraController->SetYaw(MathUtils::DegreesToRadians(yawDegrees));
	}
	ImGui::PopID();

	float pitchDegrees = MathUtils::RadiansToDegrees(gameCameraController->GetPitch());
	ImGui::PushID("Pitch");
	if (UiUtil::EditFloatSliderWithInput("Pitch", pitchDegrees, -89.0f, 89.0f, "%.1f deg", "%.2f"))
	{
		float dummyFov = 60.0f;
		float dummySpeed = 0.15f;
		ClampCameraUiValues(pitchDegrees, dummyFov, dummySpeed);
		gameCameraController->SetPitch(MathUtils::DegreesToRadians(pitchDegrees));
	}
	ImGui::PopID();

	float fovYDegrees = gameCameraController->GetFovYDegrees();
	ImGui::PushID("FOV");
	if (UiUtil::EditFloatSliderWithInput("FOV", fovYDegrees, 1.0f, 179.0f, "%.1f deg", "%.1f"))
	{
		float dummyPitch = 0.0f;
		float dummySpeed = 0.0f;
		ClampCameraUiValues(dummyPitch, fovYDegrees, dummySpeed);
		gameCameraController->SetFovYDegrees(fovYDegrees);
	}
	ImGui::PopID();

	float moveSpeed = gameCameraController->GetMoveSpeed();
	ImGui::PushID("Speed");
	if (UiUtil::EditFloatSliderWithInput("Speed", moveSpeed, 0.0001f, 10.0f, "%.4f", "%.4f"))
	{
		float dummyPitch = 0.0f;
		float dummyFov = 60.0f;
		ClampCameraUiValues(dummyPitch, dummyFov, moveSpeed);
		gameCameraController->SetMoveSpeed(moveSpeed);
	}
	ImGui::PopID();

	if (!m_statusMessage.empty())
	{
		const ImVec4 color = m_bLastSaveSucceeded ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) : ImVec4(0.9f, 0.4f, 0.3f, 1.0f);
		ImGui::TextColored(color, "%s", m_statusMessage.c_str());
	}

	if (!hasActiveLevel)
	{
		ImGui::BeginDisabled();
	}

	const float buttonWidth = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x) * 0.5f;
	if (ImGui::Button("Reset To Defaults", ImVec2(buttonWidth, 0.0f)))
	{
		m_bLastSaveSucceeded = m_levelManager->ResetActiveLevelCamera();
		m_statusMessage = m_bLastSaveSucceeded ? "Reset camera to level defaults" : "Failed to reset camera";
	}
	ImGui::SameLine();
	if (ImGui::Button("Save Defaults", ImVec2(buttonWidth, 0.0f)))
	{
		const CameraDesc cameraDesc = gameCameraController->CaptureCurrentCameraDesc();
		m_bLastSaveSucceeded = m_levelManager->SaveActiveLevelCameraDefaults(cameraDesc);
		m_statusMessage = m_bLastSaveSucceeded ? "Saved camera defaults" : "Failed to save camera defaults";
	}

	if (!hasActiveLevel)
	{
		ImGui::EndDisabled();
	}
}