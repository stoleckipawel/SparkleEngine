#include "PCH.h"
#include "Editor/Public/ExternalCapture/ExternalCaptureOverlay.h"
#include "Editor/Public/Icons/EditorIconService.h"
#include "EditorIconAssets.h"

#include <imgui.h>
#include <utility>

struct CaptureButtonPresentation final
{
	const EditorIconAsset* Icon = nullptr;
	const char* Label = nullptr;
};

static CaptureButtonPresentation GetCaptureButtonPresentation(ExternalCaptureProvider provider) noexcept
{
	switch (provider)
	{
		case ExternalCaptureProvider::Pix:
			return {&EditorIconAssets::ExternalTools::Pix, "PIX"};
		case ExternalCaptureProvider::NsightGraphics:
			return {&EditorIconAssets::ExternalTools::NsightGraphics, "Nsight Graphics (Experimental SDK)"};
		case ExternalCaptureProvider::RenderDoc:
			return {&EditorIconAssets::ExternalTools::RenderDoc, "RenderDoc"};
		default:
			return {};
	}
}

static void DrawCaptureTooltip(const ExternalCaptureSnapshot& snapshot, const char* label)
{
	if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		return;
	}

	ImGui::BeginTooltip();
	ImGui::Text("Capture a GPU frame with %s", label);
	if (snapshot.State == ExternalCaptureState::Queued || snapshot.State == ExternalCaptureState::Capturing)
	{
		ImGui::TextUnformatted("Capturing...");
	}
	else if (snapshot.State == ExternalCaptureState::Unavailable || snapshot.State == ExternalCaptureState::Failed
	    || snapshot.State == ExternalCaptureState::Quarantined)
	{
		ImGui::TextUnformatted(snapshot.Message.c_str());
	}
	ImGui::EndTooltip();
}

class ExternalCaptureOverlay final : public ViewportOverlay
{
public:
	explicit ExternalCaptureOverlay(std::unique_ptr<EditorExternalCaptureCommands> commands) noexcept :

	    m_commands(std::move(commands))
	{
	}

	float MeasureWidth() const noexcept override { return 2.0f * (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y); }

	void Draw(EditorIconService& icons, bool disableInteraction) noexcept override;

private:
	std::unique_ptr<EditorExternalCaptureCommands> m_commands;
};

void ExternalCaptureOverlay::Draw(EditorIconService& icons, bool disableInteraction) noexcept
{
	const auto snapshot = m_commands->Observe();
	const auto presentation = GetCaptureButtonPresentation(snapshot.Provider);
	if (presentation.Icon == nullptr)
	{
		return;
	}

	ImGui::BeginDisabled(disableInteraction || !CanRequestExternalCapture(snapshot.State));
	if (icons.DrawButton(*presentation.Icon, presentation.Label, ImGui::GetFontSize() * 2.0f))
	{
		(void) m_commands->Request();
	}
	ImGui::EndDisabled();

	DrawCaptureTooltip(snapshot, presentation.Label);
}

std::unique_ptr<ViewportOverlay> CreateExternalCaptureOverlay(std::unique_ptr<EditorExternalCaptureCommands> commands)
{
	if (commands == nullptr || commands->Observe().Provider == ExternalCaptureProvider::None)
	{
		return {};
	}

	return std::make_unique<ExternalCaptureOverlay>(std::move(commands));
}
