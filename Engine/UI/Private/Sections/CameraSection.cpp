#include "PCH.h"
#include "Sections/CameraSection.h"

#include "Core/Public/Math/MathUtils.h"
#include "Runtime/Level/LevelManager.h"
#include "Scene/Camera/CameraController.h"

#include <imgui.h>

CameraSection::CameraSection(LevelManager& levelManager) noexcept :
    m_levelManager(&levelManager)
{
}

void CameraSection::BuildUI()
{
	if (m_levelManager == nullptr)
	{
		ImGui::TextDisabled("Camera controls unavailable");
		return;
	}

	CameraController* cameraController = m_levelManager->GetCameraController();
	if (cameraController == nullptr)
	{
		ImGui::TextDisabled("Camera controller unavailable");
		return;
	}

	const bool hasActiveLevel = m_levelManager->HasActiveLevel();

	DirectX::XMFLOAT3 position = cameraController->GetPosition();
	float positionValues[3] = {position.x, position.y, position.z};
	if (ImGui::DragFloat3("Position", positionValues, 0.05f))
	{
		cameraController->SetPosition({positionValues[0], positionValues[1], positionValues[2]});
	}

	float yawDegrees = MathUtils::RadiansToDegrees(cameraController->GetYaw());
	if (ImGui::DragFloat("Yaw", &yawDegrees, 0.5f))
	{
		cameraController->SetYaw(MathUtils::DegreesToRadians(yawDegrees));
	}

	float pitchDegrees = MathUtils::RadiansToDegrees(cameraController->GetPitch());
	if (ImGui::DragFloat("Pitch", &pitchDegrees, 0.5f, -89.0f, 89.0f, "%.2f deg"))
	{
		cameraController->SetPitch(MathUtils::DegreesToRadians(pitchDegrees));
	}

	float fovYDegrees = cameraController->GetFovYDegrees();
	if (ImGui::SliderFloat("FOV", &fovYDegrees, 1.0f, 179.0f, "%.1f deg"))
	{
		cameraController->SetFovYDegrees(fovYDegrees);
	}

	float moveSpeed = cameraController->GetMoveSpeed();
	if (ImGui::DragFloat("Speed", &moveSpeed, 0.01f, 0.01f, 10.0f, "%.3f"))
	{
		cameraController->SetMoveSpeed(moveSpeed);
	}

	if (!m_statusMessage.empty())
	{
		const ImVec4 color = m_bLastSaveSucceeded ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) : ImVec4(0.9f, 0.4f, 0.3f, 1.0f);
		ImGui::TextColored(color, "%s", m_statusMessage.c_str());
	}

	if (!hasActiveLevel)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Save Camera Defaults", ImVec2(-1.0f, 0.0f)))
	{
		const CameraDesc cameraDesc = cameraController->CaptureCurrentCameraDesc();
		m_bLastSaveSucceeded = m_levelManager->SaveActiveLevelCameraDefaults(cameraDesc);
		m_statusMessage = m_bLastSaveSucceeded
		    ? "Saved camera defaults"
		    : "Failed to save camera defaults";
	}

	if (!hasActiveLevel)
	{
		ImGui::EndDisabled();
	}
}