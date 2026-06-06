#include "PCH.h"
#include "Panels/SceneInspectorPanel.h"

#include "Core/Public/Math/MathUtils.h"
#include "Core/Public/Strings/StringUtils.h"
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
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include <imgui.h>

namespace
{
	UiUtil::EditorIcon BuildSelectionIcon(const SceneObjectSelection* selection) noexcept
	{
		if (selection == nullptr)
		{
			return UiUtil::EditorIcon::None;
		}

		switch (selection->type)
		{
			case SceneObjectType::Camera:
				return UiUtil::EditorIcon::Camera;
			case SceneObjectType::DirectionalLight:
				return UiUtil::EditorIcon::DirectionalLight;
			case SceneObjectType::Mesh:
				return UiUtil::EditorIcon::StaticMesh;
			case SceneObjectType::None:
			default:
				return UiUtil::EditorIcon::None;
		}
	}

	bool MatchesFilterText(const std::string& filterText, const char* text) noexcept
	{
		return filterText.empty() || (text != nullptr && Strings::ContainsIgnoreCase(text, filterText));
	}

	ImU32 WithAlphaU32(ImVec4 color, float alpha) noexcept
	{
		color.w *= alpha;
		return ImGui::ColorConvertFloat4ToU32(color);
	}
}  // namespace

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
			if (m_gameScene != nullptr && m_selection->index < m_gameScene->GetCameras().GetCameraCount())
			{
				const std::string& name = m_gameScene->GetCameras().GetCameraEntries()[m_selection->index].name;
				return name.empty() ? "Camera " + std::to_string(m_selection->index + 1) : name;
			}
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
	const std::string title = BuildSelectionTitle();
	const char* subtitle = BuildSelectionSubtitle();
	const UiUtil::EditorIcon icon = BuildSelectionIcon(m_selection);

	constexpr float kHeaderHeight = 30.0f;
	constexpr float kHeaderPaddingX = 8.0f;
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 start = ImGui::GetCursorScreenPos();
	const ImVec2 end(start.x + width, start.y + kHeaderHeight);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::HeaderBackgroundActive()));
	drawList->AddRectFilled(start, ImVec2(start.x + 3.0f, end.y), ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::Accent()));
	drawList->AddLine(ImVec2(start.x, start.y), ImVec2(end.x, start.y), WithAlphaU32(SparkleUiPalette::Border(), 0.52f));
	drawList->AddLine(ImVec2(start.x, end.y - 1.0f), ImVec2(end.x, end.y - 1.0f), WithAlphaU32(SparkleUiPalette::Border(), 0.58f));

	ImGui::InvisibleButton("##DetailsSelectionHeader", ImVec2(width, kHeaderHeight));

	ImGui::SetCursorScreenPos(ImVec2(start.x + kHeaderPaddingX + 10.0f, start.y + 7.0f));
	ImGui::TextUnformatted(UiUtil::GetEditorIconGlyph(icon));
	ImGui::SameLine(0.0f, 8.0f);
	ImGui::TextUnformatted(title.c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("%s", subtitle);

	ImGui::SetCursorScreenPos(ImVec2(start.x, end.y));
	BuildDetailsToolbar();
}

void SceneInspectorPanel::BuildDetailsToolbar() noexcept
{
	char filterBuffer[128] = {};
	const std::size_t copyLength = (std::min) (m_filterText.size(), sizeof(filterBuffer) - 1);
	if (copyLength > 0)
	{
		std::copy_n(m_filterText.data(), copyLength, filterBuffer);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.058f, 0.064f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.075f, 0.080f, 0.090f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.070f, 0.095f, 0.130f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, WithAlphaU32(SparkleUiPalette::Border(), 0.70f));
	ImGui::SetNextItemWidth(-1.0f);
	const std::string searchHint = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Search, "Search");
	if (ImGui::InputTextWithHint("##DetailsFilter", searchHint.c_str(), filterBuffer, sizeof(filterBuffer)))
	{
		m_filterText = filterBuffer;
	}
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);
}

void SceneInspectorPanel::SetWidth(float widthPixels) noexcept
{
	m_widthPixels = widthPixels;
}

void SceneInspectorPanel::SetTopInset(float topInsetPixels) noexcept
{
	m_topInsetPixels = topInsetPixels;
}

void SceneInspectorPanel::ClampCameraUiValues(float& fovYDegrees, float& moveSpeed) noexcept
{
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
	ImGui::TextDisabled("Select an object from the scene outliner to inspect its properties.");
}

void SceneInspectorPanel::BuildSelectionInspector() noexcept
{
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
}

bool SceneInspectorPanel::ShouldShowDetailsCategory(SceneInspectorFilter, const char* title, const char* keywords) const noexcept
{
	return MatchesFilterText(m_filterText, title) || MatchesFilterText(m_filterText, keywords);
}

void SceneInspectorPanel::BuildCameraInspector() noexcept
{
	SceneCamera& sceneCamera = m_gameScene->GetCameras().GetActiveCamera();
	CameraComponent& cameraComponent = sceneCamera.GetCameraComponent();

	BuildCameraTransformCategory(cameraComponent);
	BuildCameraCategory(cameraComponent);
	BuildCameraMovementCategory(sceneCamera);
	BuildCameraAdvancedParametersCategory(cameraComponent);
}

void SceneInspectorPanel::BuildCameraTransformCategory(CameraComponent& cameraComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Transform, "Transform", "location rotation scale position"))
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

void SceneInspectorPanel::BuildCameraCategory(CameraComponent& cameraComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::General, "Camera", "field of view near clip far clip aspect ratio"))
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
		float dummySpeed = 0.15f;
		ClampCameraUiValues(fovYDegrees, dummySpeed);
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

void SceneInspectorPanel::BuildCameraAdvancedParametersCategory(CameraComponent& cameraComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::General, "Advanced", "visible visibility hidden"))
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

void SceneInspectorPanel::BuildCameraMovementCategory(SceneCamera& sceneCamera) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::General, "Movement", "move speed navigation"))
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
		float dummyFov = 60.0f;
		ClampCameraUiValues(dummyFov, moveSpeed);
		settings.moveSpeed = moveSpeed;
		sceneCamera.SetSettings(settings);
	}

	UiUtil::EndDetailsCategory();
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
	BuildDirectionalLightTransformCategory(lightDesc);
	BuildDirectionalLightCategory(light, lightDesc);
	BuildDirectionalLightAdvancedParametersCategory(light);
	light.ApplyDesc(lightDesc);
}

void SceneInspectorPanel::BuildDirectionalLightTransformCategory(DirectionalLightDesc& lightDesc) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Transform, "Transform", "rotation direction transform"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Transform"))
	{
		return;
	}

	DirectX::XMFLOAT3 rotationDegrees = MathUtils::DirectionToRotationDegrees(lightDesc.direction);
	float rotationValues[3] = {rotationDegrees.x, rotationDegrees.y, rotationDegrees.z};
	const DirectX::XMFLOAT3 defaultRotation = MathUtils::DirectionToRotationDegrees(DirectX::XMFLOAT3{0.0f, -1.0f, 0.0f});
	const float defaultRotationValues[3] = {defaultRotation.x, defaultRotation.y, defaultRotation.z};
	if (UiUtil::EditDetailsFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f", defaultRotationValues))
	{
		lightDesc.direction =
		    MathUtils::RotationDegreesToDirection(DirectX::XMFLOAT3{rotationValues[0], rotationValues[1], rotationValues[2]});
	}

	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildDirectionalLightCategory(DirectionalLightComponent& light, DirectionalLightDesc& lightDesc) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Rendering, "Light", "direction intensity color cast shadow rendering"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Light"))
	{
		return;
	}

	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	const float defaultDirection[3] = {0.0f, -1.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Direction", directionValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultDirection))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
	}

	float intensity = lightDesc.intensity;
	constexpr float kDefaultIntensity = 1.0f;
	if (UiUtil::EditDetailsFloat("Intensity", intensity, 0.05f, kIntensitySliderMin, kIntensitySliderMax, "%.3f", &kDefaultIntensity))
	{
		DirectX::XMFLOAT3 dummyColor = {1.0f, 1.0f, 1.0f};
		ClampLightingUiValues(dummyColor, intensity);
		lightDesc.intensity = intensity;
	}

	float colorValues[3] = {lightDesc.color.x, lightDesc.color.y, lightDesc.color.z};
	const float defaultColor[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsColor3("Color", colorValues, defaultColor))
	{
		DirectX::XMFLOAT3 clampedColor = {colorValues[0], colorValues[1], colorValues[2]};
		float dummyIntensity = 1.0f;
		ClampLightingUiValues(clampedColor, dummyIntensity);
		lightDesc.color = clampedColor;
	}

	bool castShadow = lightDesc.castShadow;
	constexpr bool kDefaultCastShadow = true;
	if (UiUtil::EditDetailsCheckbox("Cast Shadow", castShadow, &kDefaultCastShadow))
	{
		lightDesc.castShadow = castShadow;
	}
	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildDirectionalLightAdvancedParametersCategory(DirectionalLightComponent& light) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Rendering, "Advanced", "visible visibility hidden"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Advanced", false))
	{
		return;
	}

	constexpr bool kDefaultVisible = true;
	bool visible = light.IsVisible();
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &kDefaultVisible))
	{
		light.SetVisible(visible);
	}

	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildMeshTransformCategory(MeshComponent& meshComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Transform, "Transform", "location rotation scale transform"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Transform"))
	{
		return;
	}

	Transform& transform = meshComponent.GetTransform();
	DirectX::XMFLOAT3 translation = transform.GetTranslation();
	float translationValues[3] = {translation.x, translation.y, translation.z};
	const float defaultTranslation[3] = {0.0f, 0.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Location", translationValues, 0.05f, kPositionSliderMin, kPositionSliderMax, "%.3f", defaultTranslation))
	{
		transform.SetTranslation({translationValues[0], translationValues[1], translationValues[2]});
	}

	DirectX::XMFLOAT3 rotationEuler = transform.GetRotationEuler();
	const DirectX::XMFLOAT3 rotationDegrees = MathUtils::RadiansToDegrees(rotationEuler);
	float rotationValues[3] = {rotationDegrees.x, rotationDegrees.y, rotationDegrees.z};
	const float defaultRotation[3] = {0.0f, 0.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f", defaultRotation))
	{
		transform.SetRotationEuler(MathUtils::DegreesToRadians(DirectX::XMFLOAT3{rotationValues[0], rotationValues[1], rotationValues[2]}));
	}

	DirectX::XMFLOAT3 scale = transform.GetScale();
	float scaleValues[3] = {scale.x, scale.y, scale.z};
	const float defaultScale[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsFloat3("Scale", scaleValues, 0.01f, kScaleSliderMin, kScaleSliderMax, "%.3f", defaultScale))
	{
		transform.SetScale({scaleValues[0], scaleValues[1], scaleValues[2]});
	}

	UiUtil::EndDetailsCategory();
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

	BuildMeshTransformCategory(*meshComponent);
	BuildStaticMeshCategory(*mesh, *meshComponent);
	BuildStaticMeshAdvancedCategory(*mesh);
	BuildMeshAdvancedParametersCategory(*meshComponent);
	BuildMeshMaterialsCategory(*meshComponent);
}

void SceneInspectorPanel::BuildStaticMeshCategory(const Mesh& mesh, MeshComponent& meshComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Rendering, "Static Mesh", "type mesh asset rendering"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Static Mesh"))
	{
		return;
	}

	const bool isCookedMesh = dynamic_cast<const CookedMesh*>(&mesh) != nullptr;
	UiUtil::DrawDetailsAssetRow(
	    "Mesh",
	    UiUtil::EditorIcon::StaticMesh,
	    isCookedMesh ? "Cooked Static Mesh" : "Procedural Mesh",
	    isCookedMesh ? "Cooked asset" : "Generated geometry");
	UiUtil::DrawDetailsValueRow("Type", isCookedMesh ? "Cooked" : "Procedural");
	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildStaticMeshAdvancedCategory(const Mesh& mesh) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Rendering, "Advanced Mesh Data", "vertices indices mesh data statistics"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Advanced Mesh Data", false))
	{
		return;
	}

	char buffer[64] = {};
	const MeshData& meshData = mesh.GetMeshData();
	std::snprintf(buffer, sizeof(buffer), "%u", meshData.GetVertexCount());
	UiUtil::DrawDetailsValueRow("Vertices", buffer);
	std::snprintf(buffer, sizeof(buffer), "%u", meshData.GetIndexCount());
	UiUtil::DrawDetailsValueRow("Indices", buffer);
	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildMeshAdvancedParametersCategory(MeshComponent& meshComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Rendering, "Advanced", "visible visibility hidden"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Advanced", false))
	{
		return;
	}

	constexpr bool kDefaultVisible = true;
	bool visible = meshComponent.IsVisible();
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &kDefaultVisible))
	{
		meshComponent.SetVisible(visible);
	}

	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildMeshMaterialsCategory(const MeshComponent& meshComponent) noexcept
{
	if (!ShouldShowDetailsCategory(SceneInspectorFilter::Materials, "Materials", "element material slot surface"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Materials"))
	{
		return;
	}

	char buffer[64] = {};
	const MaterialHandle materialHandle = meshComponent.GetMaterialHandle();
	std::snprintf(buffer, sizeof(buffer), "Material %u", materialHandle.IsValid() ? materialHandle.GetIndex() : 0u);
	UiUtil::DrawDetailsAssetRow("Element 0", UiUtil::EditorIcon::Material, buffer, "Material slot");
	UiUtil::EndDetailsCategory();
}

void SceneInspectorPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();

	constexpr float kMinWidth = 320.0f;
	const float kMaxWidth = (std::max) (kMinWidth + 1.0f, io.DisplaySize.x * 0.7f);
	m_widthPixels = std::clamp(m_widthPixels, kMinWidth, kMaxWidth);
	const float panelHeight = (std::max) (1.0f, io.DisplaySize.y - m_topInsetPixels);

	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - m_widthPixels, m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, panelHeight), ImGuiCond_Once);
	ImGui::SetNextWindowSizeConstraints(ImVec2(kMinWidth, panelHeight), ImVec2(kMaxWidth, panelHeight));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(
	    "Inspector",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleVar();

	m_widthPixels = ImGui::GetWindowWidth();

	if (m_gameScene == nullptr || m_selection == nullptr)
	{
		ImGui::TextDisabled("Scene inspector unavailable");
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(disableInteraction);

	constexpr float kContentPad = 8.0f;
	ImGui::Indent(kContentPad);

	constexpr ImGuiTabBarFlags kTabBarFlags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyResizeDown;
	if (ImGui::BeginTabBar("##InspectorTabs", kTabBarFlags))
	{
		if (ImGui::BeginTabItem("Details"))
		{
			ImGui::Spacing();
			BuildSelectionInspector();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Unindent(kContentPad);

	ImGui::EndDisabled();

	ImGui::End();
}
