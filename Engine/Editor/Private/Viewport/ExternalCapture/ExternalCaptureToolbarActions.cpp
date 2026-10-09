#include "PCH.h"
#include "Editor/Public/ExternalCapture/ExternalCaptureToolbar.h"

#include <imgui.h>

#include <array>

namespace
{
	class ExternalCaptureToolbarActions final : public ViewportToolbarActions
	{
	public:
		explicit ExternalCaptureToolbarActions(ExternalCaptureToolRequests requested) noexcept :
		    m_requested(requested)
		{
		}
		float MeasureWidth() const noexcept override;
		void Draw(bool disableInteraction) noexcept override;

	private:
		const ExternalCaptureToolRequests m_requested;
	};

	struct CaptureToolButton final
	{
		bool ExternalCaptureToolRequests::* Requested;
		const char* Label;
	};

	static constexpr std::array captureToolButtons{
	    CaptureToolButton{&ExternalCaptureToolRequests::Pix, "PIX"},
	    CaptureToolButton{&ExternalCaptureToolRequests::Nsight, "Nsight"},
	    CaptureToolButton{&ExternalCaptureToolRequests::RenderDoc, "RenderDoc"}};

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
				width += ImGui::CalcTextSize(button.Label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
			}
		}
		return width;
	}

	void ExternalCaptureToolbarActions::Draw(bool disableInteraction) noexcept
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
			ImGui::Button(button.Label);
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
} // namespace

std::unique_ptr<ViewportToolbarActions> CreateExternalCaptureToolbarActions(ExternalCaptureToolRequests requested)
{
	if (!requested.Pix && !requested.Nsight && !requested.RenderDoc)
	{
		return {};
	}
	return std::make_unique<ExternalCaptureToolbarActions>(requested);
}
