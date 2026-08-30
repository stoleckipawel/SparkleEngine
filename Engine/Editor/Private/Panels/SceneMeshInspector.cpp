#include "PCH.h"
#include "Panels/SceneMeshInspector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/Transactions/EditorTransactionHistory.h"
#include "Util/UiUtil.h"
#include "World/WorldReadView.h"

#include <cstdio>

void SceneMeshInspector::Build(
    const WorldMeshReadData& mesh,
    EditorTransactionHistory& transactionHistory,
    std::uint64_t generation,
    const std::string& filter) noexcept
{
	BuildTransformCategory(filter, mesh, transactionHistory, generation);
	BuildStaticMeshCategory(filter, mesh);
	BuildAdvancedParametersCategory(filter, mesh, transactionHistory, generation);
	BuildMaterialsCategory(filter, mesh);
}

void SceneMeshInspector::BuildTransformCategory(
    const std::string& filter,
    const WorldMeshReadData& mesh,
    EditorTransactionHistory& transactionHistory,
    std::uint64_t generation) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Transform", "location rotation scale transform")
	    || !UiUtil::BeginDetailsCategory("Transform"))
		return;
	Transform after = mesh.LocalTransform;
	bool changed = false;
	auto translation = after.GetTranslation();
	float p[3] = {translation.x, translation.y, translation.z};
	const float dp[3] = {};
	if (UiUtil::EditDetailsFloat3("Location", p, 0.05f, kPositionSliderMin, kPositionSliderMax, "%.3f", dp))
	{
		after.SetTranslation({p[0], p[1], p[2]});
		changed = true;
	}
	auto degrees = MathUtils::RadiansToDegrees(after.GetRotationEuler());
	float r[3] = {degrees.x, degrees.y, degrees.z};
	const float dr[3] = {};
	if (UiUtil::EditDetailsFloat3("Rotation", r, 0.1f, -360.0f, 360.0f, "%.2f", dr))
	{
		after.SetRotationEuler(MathUtils::DegreesToRadians(DirectX::XMFLOAT3{r[0], r[1], r[2]}));
		changed = true;
	}
	auto scale = after.GetScale();
	float s[3] = {scale.x, scale.y, scale.z};
	const float ds[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsFloat3("Scale", s, 0.01f, kScaleSliderMin, kScaleSliderMax, "%.3f", ds))
	{
		after.SetScale({s[0], s[1], s[2]});
		changed = true;
	}
	if (changed)
		(void) transactionHistory.Execute(
		    {0, SetLocalTransformCommand{mesh.Entity, after}},
		    {0, SetLocalTransformCommand{mesh.Entity, mesh.LocalTransform}},
		    generation,
		    "mesh-transform");
	UiUtil::EndDetailsCategory();
}

void SceneMeshInspector::BuildStaticMeshCategory(const std::string& filter, const WorldMeshReadData& mesh) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Mesh", "type asset rendering skeletal static") || !UiUtil::BeginDetailsCategory("Mesh"))
		return;
	const bool skeletal = mesh.Kind == SceneMeshKind::Skeletal;
	UiUtil::DrawDetailsAssetRow(
	    "Mesh",
	    UiUtil::EditorIcon::StaticMesh,
	    skeletal ? "Cooked Skeletal Mesh" : "Cooked Static Mesh",
	    "Generation-pinned asset reference");
	UiUtil::DrawDetailsValueRow("Type", skeletal ? "Skeletal" : "Static");
	char value[64] = {};
	std::snprintf(value, sizeof(value), "%llu", static_cast<unsigned long long>(mesh.MeshAssetId));
	UiUtil::DrawDetailsValueRow("Asset Id", value);
	UiUtil::EndDetailsCategory();
}

void SceneMeshInspector::BuildAdvancedParametersCategory(
    const std::string& filter,
    const WorldMeshReadData& mesh,
    EditorTransactionHistory& transactionHistory,
    std::uint64_t generation) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Advanced", "visible visibility hidden") || !UiUtil::BeginDetailsCategory("Advanced", false))
		return;
	bool visible = mesh.Visible;
	const bool defaultVisible = true;
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &defaultVisible))
		(void) transactionHistory.Execute(
		    {0, SetEntityVisibilityCommand{mesh.Entity, visible}},
		    {0, SetEntityVisibilityCommand{mesh.Entity, mesh.Visible}},
		    generation,
		    "mesh-visibility");
	UiUtil::EndDetailsCategory();
}

void SceneMeshInspector::BuildMaterialsCategory(const std::string& filter, const WorldMeshReadData& mesh) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Materials", "element material slot surface") || !UiUtil::BeginDetailsCategory("Materials"))
		return;
	char buffer[64] = {};
	std::snprintf(buffer, sizeof(buffer), "Material %u", mesh.Material.IsValid() ? mesh.Material.GetIndex() : 0u);
	UiUtil::DrawDetailsAssetRow("Element 0", UiUtil::EditorIcon::Material, buffer, "Material slot");
	UiUtil::EndDetailsCategory();
}
