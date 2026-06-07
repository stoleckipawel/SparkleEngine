#include "PCH.h"
#include "Panels/SceneMeshInspector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/GameScene.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Scene/Transform.h"
#include "Util/UiUtil.h"

#include <cstdio>

void SceneMeshInspector::Build(GameScene& gameScene, std::size_t meshIndex, const std::string& filterText) noexcept
{
	if (meshIndex >= gameScene.GetMeshes().GetMeshCount())
	{
		UiUtil::DrawDetailsEmptyState();
		return;
	}

	MeshComponent* meshComponent = gameScene.GetMeshes().GetMeshComponent(meshIndex);
	if (meshComponent == nullptr)
	{
		UiUtil::DrawDetailsEmptyState();
		return;
	}

	Mesh* mesh = meshComponent->GetMesh();
	if (mesh == nullptr)
	{
		UiUtil::DrawDetailsEmptyState();
		return;
	}

	BuildTransformCategory(filterText, *meshComponent);
	BuildStaticMeshCategory(filterText, *mesh, *meshComponent);
	BuildStaticMeshAdvancedCategory(filterText, *mesh);
	BuildAdvancedParametersCategory(filterText, *meshComponent);
	BuildMaterialsCategory(filterText, *meshComponent);
}

void SceneMeshInspector::BuildTransformCategory(const std::string& filterText, MeshComponent& meshComponent) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Transform", "location rotation scale transform"))
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

void SceneMeshInspector::BuildStaticMeshCategory(const std::string& filterText, const Mesh& mesh, MeshComponent& meshComponent) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Static Mesh", "type mesh asset rendering"))
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

void SceneMeshInspector::BuildStaticMeshAdvancedCategory(const std::string& filterText, const Mesh& mesh) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Advanced Mesh Data", "vertices indices mesh data statistics"))
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

void SceneMeshInspector::BuildAdvancedParametersCategory(const std::string& filterText, MeshComponent& meshComponent) noexcept
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
	bool visible = meshComponent.IsVisible();
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &kDefaultVisible))
	{
		meshComponent.SetVisible(visible);
	}

	UiUtil::EndDetailsCategory();
}

void SceneMeshInspector::BuildMaterialsCategory(const std::string& filterText, const MeshComponent& meshComponent) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Materials", "element material slot surface"))
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
