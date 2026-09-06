#include "PCH.h"

#include "Viewport/ViewportCameraProperties.h"

#include "Panels/ExposureSettingsEditor.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Viewport/EditorViewportSession.h"

#include <imgui.h>

#include <cfloat>

class ViewportCameraPropertyTable final
{
public:
	static bool Begin() noexcept
	{
		if (!ImGui::BeginTable("##ViewportCameraCore", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
		{
			return false;
		}
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 175.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	}

	static bool DrawFloat(
	    const char* id,
	    const char* label,
	    float& value,
	    float speed,
	    float minimum,
	    float maximum,
	    const char* format,
	    ImGuiSliderFlags flags = ImGuiSliderFlags_None) noexcept
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		return ImGui::DragFloat(id, &value, speed, minimum, maximum, format, flags);
	}
};

void ViewportCameraProperties::OpenPopup() noexcept
{
	ImGui::OpenPopup("##ViewportCameraProperties");
}

void ViewportCameraProperties::BuildPopup(
    EditorViewportSession& viewportSession,
    const EngineRenderingSettingsState& renderingDefaults,
    bool disableInteraction) noexcept
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(430.0f, 0.0f), ImVec2(560.0f, 720.0f));
	if (!ImGui::BeginPopup("##ViewportCameraProperties"))
	{
		return;
	}
	if (disableInteraction)
	{
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	const EditorViewportSettingsState& settings = viewportSession.GetSettings();
	ImGui::TextDisabled("Viewport Camera");
	ImGui::Separator();
	if (ViewportCameraPropertyTable::Begin())
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Projection");
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		const char* projectionPreview = settings.ProjectionKind == CameraProjectionKind::Orthographic ? "Orthographic" : "Perspective";
		if (ImGui::BeginCombo("##ViewportProjection", projectionPreview))
		{
			if (ImGui::Selectable("Perspective", settings.ProjectionKind == CameraProjectionKind::Perspective))
			{
				viewportSession.SetProjectionKind(CameraProjectionKind::Perspective);
			}
			if (ImGui::Selectable("Orthographic", settings.ProjectionKind == CameraProjectionKind::Orthographic))
			{
				viewportSession.SetProjectionKind(CameraProjectionKind::Orthographic);
			}
			ImGui::EndCombo();
		}

		float moveSpeed = settings.Navigation.MoveSpeedMetersPerSecond;
		if (ViewportCameraPropertyTable::DrawFloat(
		        "##ViewportMoveSpeed",
		        "Move speed (m/s)",
		        moveSpeed,
		        0.01f,
		        settings.Navigation.MinimumMoveSpeedMetersPerSecond,
		        settings.Navigation.MaximumMoveSpeedMetersPerSecond,
		        "%.4f",
		        ImGuiSliderFlags_Logarithmic))
		{
			viewportSession.SetMoveSpeed(moveSpeed);
		}

		float rotationSpeed = settings.Navigation.RotationSpeedDegreesPerPixel;
		if (ViewportCameraPropertyTable::DrawFloat(
		        "##ViewportRotationSpeed",
		        "Rotation speed (deg/px)",
		        rotationSpeed,
		        0.005f,
		        0.001f,
		        10.0f,
		        "%.4f",
		        ImGuiSliderFlags_Logarithmic))
		{
			viewportSession.SetRotationSpeed(rotationSpeed);
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Invert Y");
		ImGui::TableSetColumnIndex(1);
		bool invertY = settings.Navigation.InvertY;
		if (ImGui::Checkbox("##ViewportInvertY", &invertY))
		{
			viewportSession.SetInvertY(invertY);
		}

		if (settings.ProjectionKind == CameraProjectionKind::Orthographic)
		{
			float orthographicHeight = settings.OrthographicHeightMeters;
			if (ViewportCameraPropertyTable::DrawFloat(
			        "##ViewportOrthographicHeight",
			        "Ortho height (m)",
			        orthographicHeight,
			        0.1f,
			        0.001f,
			        1000000.0f,
			        "%.3f",
			        ImGuiSliderFlags_Logarithmic))
			{
				viewportSession.SetOrthographicHeight(orthographicHeight);
			}
		}
		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Exposure");
	ImGui::TextWrapped("Unchecked properties use the current renderer defaults.");
	ViewportExposureOverrides exposureDraft = settings.Exposure;
	if (ExposureSettingsEditor::DrawOverrides(exposureDraft, renderingDefaults))
	{
		viewportSession.SetExposureOverrides(exposureDraft);
	}

	ImGui::EndPopup();
}
