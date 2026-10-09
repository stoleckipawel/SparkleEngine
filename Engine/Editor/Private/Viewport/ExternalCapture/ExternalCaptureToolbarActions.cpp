#include "PCH.h"
#include "Editor/Public/ExternalCapture/ExternalCaptureToolbar.h"
#include "Editor/Public/Icons/EditorIconService.h"
#include "EditorIconAssets.h"

#include <imgui.h>

#include <array>

class ExternalCaptureToolbarActions final : public ViewportToolbarActions
{
public:
	explicit ExternalCaptureToolbarActions(ExternalCaptureToolRequests requested) noexcept :
	    m_requested(requested)
	{
	}
	float MeasureWidth() const noexcept override;
	void Draw(EditorIconService& icons, bool disableInteraction) noexcept override;

private:
	const ExternalCaptureToolRequests m_requested;
};

struct CaptureToolButton final
{
	bool ExternalCaptureToolRequests::* Requested;
	const char* Label;
	const EditorIconAsset* Icon;
};

static constexpr std::array captureToolButtons{
    CaptureToolButton{&ExternalCaptureToolRequests::Pix, "PIX", &EditorIconAssets::ExternalTools::Pix},
    CaptureToolButton{&ExternalCaptureToolRequests::Nsight, "Nsight Graphics", &EditorIconAssets::ExternalTools::NsightGraphics},
    CaptureToolButton{&ExternalCaptureToolRequests::RenderDoc, "RenderDoc", &EditorIconAssets::ExternalTools::RenderDoc}};

float ExternalCaptureToolbarActions::MeasureWidth() const noexcept
{
	float width = 0.0f;
	for (const CaptureToolButton& button : captureToolButtons)
	{
		if (m_requested.*button.Requested)
		{
			if (width > 0.0f)
			{
				width += ImGui::GetStyle().ItemSpacing.x;
			}
			width += ImGui::GetFrameHeight();
		}
	}
	return width;
}

void ExternalCaptureToolbarActions::Draw(EditorIconService& icons, bool disableInteraction) noexcept
{
	// No native capture producer exists yet, including when interaction is enabled.
	(void) disableInteraction;
	bool first = true;
	for (const CaptureToolButton& button : captureToolButtons)
	{
		if (!(m_requested.*button.Requested))
		{
			continue;
		}
		if (!first)
		{
			ImGui::SameLine();
		}
		first = false;
		ImGui::BeginDisabled();
		icons.DrawButton(*button.Icon, button.Label);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip(
			    "%s capture requested.\n"
			    "In-editor capture is unavailable in this build. This option has not attached the tool.\n"
			    "Launch the editor through the installed tool to capture.\n"
			    "Instructions: Docs/Engineering/Verification/ExternalProfiling.md",
			    button.Label);
		}
	}
}

std::unique_ptr<ViewportToolbarActions> CreateExternalCaptureToolbarActions(ExternalCaptureToolRequests requested)
{
	if (!requested.Pix && !requested.Nsight && !requested.RenderDoc)
	{
		return {};
	}
	return std::make_unique<ExternalCaptureToolbarActions>(requested);
}
