#include "PCH.h"
#include "Panels/SceneCameraInspector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/Transactions/EditorTransactionManager.h"
#include "Util/UiUtil.h"
#include "World/WorldReadView.h"

#include <cstdio>

class SceneCameraEditSubmission final
{
  public:
	static void Submit(
	    EditorTransactionManager& transactions,
	    std::uint64_t generation,
	    WorldEditPayload after,
	    WorldEditPayload before,
	    const char* key)
	{
		(void) transactions.Execute({0, std::move(after)}, {0, std::move(before)}, generation, key);
	}
};

void SceneCameraInspector::Build(
	const WorldCameraReadData& camera,
	EditorTransactionManager& transactions,
	std::uint64_t generation,
	const std::string& filter) noexcept
{
	BuildTransformCategory(filter, camera, transactions, generation);
	BuildCameraCategory(filter, camera, transactions, generation);
	BuildMovementCategory(filter, camera, transactions, generation);
	BuildAdvancedParametersCategory(filter, camera, transactions, generation);
}

void SceneCameraInspector::BuildTransformCategory(
	const std::string& filter,
	const WorldCameraReadData& camera,
	EditorTransactionManager& transactions,
	std::uint64_t generation) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Transform", "location rotation scale position") ||
	    !UiUtil::BeginDetailsCategory("Transform"))
	{
		return;
	}

	Transform after = camera.LocalTransform;
	bool changed = false;

	auto position = after.GetTranslation();
	float p[3] = {position.x, position.y, position.z};
	const float defaultP[3] = {0.0f, 0.0f, -4.0f};
	if (UiUtil::EditDetailsFloat3("Location", p, 0.05f, kPositionSliderMin, kPositionSliderMax, "%.3f", defaultP))
	{
		after.SetTranslation({p[0], p[1], p[2]});
		changed = true;
	}

	auto degrees = MathUtils::RadiansToDegrees(after.GetRotationEuler());
	float r[3] = {degrees.x, degrees.y, degrees.z};
	const float defaultR[3] = {};
	if (UiUtil::EditDetailsFloat3("Rotation", r, 0.1f, -360.0f, 360.0f, "%.2f", defaultR))
	{
		after.SetRotationEuler(MathUtils::DegreesToRadians(DirectX::XMFLOAT3{r[0], r[1], r[2]}));
		changed = true;
	}

	auto scale = after.GetScale();
	float s[3] = {scale.x, scale.y, scale.z};
	const float defaultS[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsFloat3("Scale", s, 0.01f, kScaleSliderMin, kScaleSliderMax, "%.3f", defaultS))
	{
		after.SetScale({s[0], s[1], s[2]});
		changed = true;
	}

	if (changed)
	{
		SceneCameraEditSubmission::Submit(
		    transactions,
		    generation,
		    SetLocalTransformCommand{camera.Entity, after},
		    SetLocalTransformCommand{camera.Entity, camera.LocalTransform},
		    "camera-transform");
	}

	UiUtil::EndDetailsCategory();
}

void SceneCameraInspector::BuildCameraCategory(
	const std::string& filter,
	const WorldCameraReadData& camera,
	EditorTransactionManager& transactions,
	std::uint64_t generation) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Camera", "field of view near clip far clip aspect ratio") ||
	    !UiUtil::BeginDetailsCategory("Camera"))
	{
		return;
	}

	CameraDesc after = camera.Description;
	bool changed = false;
	const float defaultFov = 60.0f, defaultNear = 0.1f, defaultFar = 1000.0f;

	changed |= UiUtil::EditDetailsFloat("Field Of View", after.fovYDegrees, 0.1f, 1.0f, 179.0f, "%.1f", &defaultFov);
	changed |= UiUtil::EditDetailsFloat("Near Clip", after.nearZ, 0.01f, 0.001f, after.farZ, "%.3f", &defaultNear);
	changed |= UiUtil::EditDetailsFloat("Far Clip", after.farZ, 1.0f, after.nearZ, 100000.0f, "%.3f", &defaultFar);

	char buffer[64] = {};
	std::snprintf(buffer, sizeof(buffer), "%.3f", camera.AspectRatio);
	UiUtil::DrawDetailsValueRow("Aspect Ratio", buffer);

	if (changed)
	{
		SceneCameraEditSubmission::Submit(
		    transactions,
		    generation,
		    SetCameraDescriptionCommand{camera.Entity, after},
		    SetCameraDescriptionCommand{camera.Entity, camera.Description},
		    "camera-description");
	}

	UiUtil::EndDetailsCategory();
}

void SceneCameraInspector::BuildMovementCategory(
	const std::string& filter,
	const WorldCameraReadData& camera,
	EditorTransactionManager& transactions,
	std::uint64_t generation) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Movement", "move speed navigation") ||
	    !UiUtil::BeginDetailsCategory("Movement"))
	{
		return;
	}

	CameraMovementSettings after = camera.Movement;
	const float defaultSpeed = 0.10f;
	if (UiUtil::EditDetailsFloat("Move Speed", after.moveSpeed, 0.01f, 0.0001f, 10.0f, "%.4f", &defaultSpeed))
	{
		SceneCameraEditSubmission::Submit(
		    transactions,
		    generation,
		    SetCameraMovementCommand{camera.Entity, after},
		    SetCameraMovementCommand{camera.Entity, camera.Movement},
		    "camera-movement");
	}

	UiUtil::EndDetailsCategory();
}

void SceneCameraInspector::BuildAdvancedParametersCategory(
	const std::string& filter,
	const WorldCameraReadData& camera,
	EditorTransactionManager& transactions,
	std::uint64_t generation) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filter, "Advanced", "visible visibility hidden") ||
	    !UiUtil::BeginDetailsCategory("Advanced", false))
	{
		return;
	}

	bool visible = camera.Visible;
	const bool defaultVisible = true;
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &defaultVisible))
	{
		SceneCameraEditSubmission::Submit(
		    transactions,
		    generation,
		    SetEntityVisibilityCommand{camera.Entity, visible},
		    SetEntityVisibilityCommand{camera.Entity, camera.Visible},
		    "camera-visibility");
	}

	UiUtil::EndDetailsCategory();
}
