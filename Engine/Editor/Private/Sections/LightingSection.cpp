#include "PCH.h"
#include "Sections/LightingSection.h"

#include "Level/Level.h"
#include "Runtime/Level/LevelManager.h"
#include "Scene/Lighting/DirectionalLightDesc.h"
#include "Scene/Lighting/PointLightDesc.h"
#include "Scene/Lighting/GameSceneLightingState.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

namespace
{
	constexpr float DirectionSliderMin = -1.0f;
	constexpr float DirectionSliderMax = 1.0f;
	constexpr float PositionSliderMin = -50.0f;
	constexpr float PositionSliderMax = 50.0f;
	constexpr float IntensitySliderMin = 0.0f;
	constexpr float IntensitySliderMax = 20.0f;
	constexpr float RadiusSliderMin = 0.1f;
	constexpr float RadiusSliderMax = 50.0f;

	void ClampLightingUiValues(DirectX::XMFLOAT3& color, float& intensity)
	{
		color.x = std::clamp(color.x, 0.0f, 1.0f);
		color.y = std::clamp(color.y, 0.0f, 1.0f);
		color.z = std::clamp(color.z, 0.0f, 1.0f);
		intensity = (std::max) (0.0f, intensity);
	}

	void ClampPointLightUiValues(DirectX::XMFLOAT3& color, float& intensity, float& radius)
	{
		ClampLightingUiValues(color, intensity);
		radius = (std::max) (RadiusSliderMin, radius);
	}
}  // namespace

LightingSection::LightingSection(LevelManager& levelManager) noexcept : m_levelManager(&levelManager) {}

std::vector<LightingSection::LightSelectionEntry> LightingSection::BuildSelectionEntries() const
{
	std::vector<LightSelectionEntry> entries;
	if (m_levelManager == nullptr)
	{
		return entries;
	}

	const Level* activeLevel = m_levelManager->GetActiveLevel();
	if (activeLevel == nullptr)
	{
		return entries;
	}

	entries.reserve(
	    static_cast<std::size_t>(activeLevel->GetDirectionalLightCount()) + static_cast<std::size_t>(activeLevel->GetPointLightCount()));

	for (std::size_t lightIndex = 0; lightIndex < activeLevel->GetDirectionalLightCount(); ++lightIndex)
	{
		entries.push_back({true, lightIndex, "Directional Light " + std::to_string(lightIndex + 1)});
	}

	for (std::size_t lightIndex = 0; lightIndex < activeLevel->GetPointLightCount(); ++lightIndex)
	{
		entries.push_back({false, lightIndex, "Point Light " + std::to_string(lightIndex + 1)});
	}

	return entries;
}

void LightingSection::BuildUI()
{
	if (m_levelManager == nullptr)
	{
		ImGui::TextDisabled("Lighting controls unavailable");
		return;
	}

	GameSceneLightingState* lightingState = m_levelManager->GetGameSceneLightingState();
	if (lightingState == nullptr)
	{
		ImGui::TextDisabled("Scene lighting unavailable");
		return;
	}

	const std::vector<LightSelectionEntry> selectionEntries = BuildSelectionEntries();
	if (selectionEntries.empty())
	{
		ImGui::TextDisabled("No lights are defined in the active level");
		return;
	}

	const bool hasActiveLevel = m_levelManager->HasActiveLevel();
	const ImGuiStyle& style = ImGui::GetStyle();
	const int maxSelectionIndex = static_cast<int>(selectionEntries.size()) - 1;
	m_selectedLightIndex = std::clamp(m_selectedLightIndex, 0, maxSelectionIndex);

	std::vector<const char*> selectionLabels;
	selectionLabels.reserve(selectionEntries.size());
	for (const LightSelectionEntry& entry : selectionEntries)
	{
		selectionLabels.push_back(entry.label.c_str());
	}

	ImGui::SetNextItemWidth(-1.0f);
	ImGui::Combo("##SelectedLight", &m_selectedLightIndex, selectionLabels.data(), static_cast<int>(selectionLabels.size()));

	const LightSelectionEntry& selectedEntry = selectionEntries[static_cast<std::size_t>(m_selectedLightIndex)];

	if (selectedEntry.bDirectional)
	{
		const std::size_t directionalLightIndex = selectedEntry.lightIndex;

		DirectionalLightDesc directionalLight = lightingState->GetDirectionalLight(directionalLightIndex);
		DirectX::XMFLOAT3 direction = directionalLight.direction;
		float directionValues[3] = {direction.x, direction.y, direction.z};
		ImGui::PushID("DirectionalDirection");
		if (UiUtil::EditFloat3SliderWithInput("Direction", directionValues, DirectionSliderMin, DirectionSliderMax, "%.2f", "%.3f"))
		{
			directionalLight.direction = {directionValues[0], directionValues[1], directionValues[2]};
			lightingState->SetDirectionalLight(directionalLightIndex, directionalLight);
		}
		ImGui::PopID();

		float intensity = directionalLight.intensity;
		ImGui::PushID("DirectionalIntensity");
		if (UiUtil::EditFloatSliderWithInput("Intensity", intensity, IntensitySliderMin, IntensitySliderMax, "%.2f", "%.3f"))
		{
			DirectX::XMFLOAT3 dummyColor = {1.0f, 1.0f, 1.0f};
			ClampLightingUiValues(dummyColor, intensity);
			directionalLight.intensity = intensity;
			lightingState->SetDirectionalLight(directionalLightIndex, directionalLight);
		}
		ImGui::PopID();

		DirectX::XMFLOAT3 color = directionalLight.color;
		float colorValues[3] = {color.x, color.y, color.z};
		ImGui::PushID("DirectionalColor");
		if (UiUtil::EditColor3("Color", colorValues))
		{
			DirectX::XMFLOAT3 clampedColor = {colorValues[0], colorValues[1], colorValues[2]};
			float dummyIntensity = 1.0f;
			ClampLightingUiValues(clampedColor, dummyIntensity);
			directionalLight.color = clampedColor;
			lightingState->SetDirectionalLight(directionalLightIndex, directionalLight);
		}
		ImGui::PopID();
	}
	else
	{
		const std::size_t pointLightIndex = selectedEntry.lightIndex;

		PointLightDesc pointLight = lightingState->GetPointLight(pointLightIndex);

		ImGui::PushID(static_cast<int>(pointLightIndex));
		bool enabled = pointLight.enabled;
		if (ImGui::Checkbox("Enabled", &enabled))
		{
			pointLight.enabled = enabled;
			lightingState->SetPointLight(pointLightIndex, pointLight);
		}

		float positionValues[3] = {pointLight.position.x, pointLight.position.y, pointLight.position.z};
		if (UiUtil::EditFloat3SliderWithInput("Position", positionValues, PositionSliderMin, PositionSliderMax, "%.2f", "%.3f"))
		{
			pointLight.position = {positionValues[0], positionValues[1], positionValues[2]};
			lightingState->SetPointLight(pointLightIndex, pointLight);
		}

		float pointIntensity = pointLight.intensity;
		if (UiUtil::EditFloatSliderWithInput("Intensity", pointIntensity, IntensitySliderMin, IntensitySliderMax, "%.2f", "%.3f"))
		{
			DirectX::XMFLOAT3 dummyColor = {1.0f, 1.0f, 1.0f};
			float dummyRadius = pointLight.radius;
			ClampPointLightUiValues(dummyColor, pointIntensity, dummyRadius);
			pointLight.intensity = pointIntensity;
			lightingState->SetPointLight(pointLightIndex, pointLight);
		}

		DirectX::XMFLOAT3 pointColor = pointLight.color;
		float pointColorValues[3] = {pointColor.x, pointColor.y, pointColor.z};
		if (UiUtil::EditColor3("Color", pointColorValues))
		{
			DirectX::XMFLOAT3 clampedColor = {pointColorValues[0], pointColorValues[1], pointColorValues[2]};
			float dummyIntensity = pointLight.intensity;
			float dummyRadius = pointLight.radius;
			ClampPointLightUiValues(clampedColor, dummyIntensity, dummyRadius);
			pointLight.color = clampedColor;
			lightingState->SetPointLight(pointLightIndex, pointLight);
		}

		float radius = pointLight.radius;
		if (UiUtil::EditFloatSliderWithInput("Radius", radius, RadiusSliderMin, RadiusSliderMax, "%.2f", "%.3f"))
		{
			DirectX::XMFLOAT3 dummyColor = pointLight.color;
			float dummyIntensity = pointLight.intensity;
			ClampPointLightUiValues(dummyColor, dummyIntensity, radius);
			pointLight.radius = radius;
			lightingState->SetPointLight(pointLightIndex, pointLight);
		}
		ImGui::PopID();
	}

	if (!m_statusMessage.empty())
	{
		const ImVec4 colorValue = m_bLastSaveSucceeded ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) : ImVec4(0.9f, 0.4f, 0.3f, 1.0f);
		ImGui::TextColored(colorValue, "%s", m_statusMessage.c_str());
	}

	if (!hasActiveLevel)
	{
		ImGui::BeginDisabled();
	}

	const float buttonWidth = ImGui::GetContentRegionAvail().x;
	if (ImGui::Button("Save Defaults", ImVec2(buttonWidth, 0.0f)))
	{
		m_bLastSaveSucceeded = m_levelManager->SaveActiveLevelLightingDefaults();
		m_statusMessage = m_bLastSaveSucceeded ? "Saved lighting defaults" : "Failed to save lighting defaults";
	}

	if (!hasActiveLevel)
	{
		ImGui::EndDisabled();
	}
}