#include "PCH.h"

#include "Panels/ReferencePathTracer/ReferencePathTracerOverlay.h"

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include <imgui.h>

static constexpr std::array ProgressStateLabels = {"Inactive", "Resetting", "Accumulating", "Paused", "Unavailable", "Complete"};
static constexpr std::array ProgressReasonLabels = {
    "None",
    "View changed",
    "Camera changed",
    "Scene geometry changed",
    "Deformation changed",
    "Material changed",
    "Lighting changed",
    "Environment changed",
    "Shader changed",
    "Execution route changed",
    "Backend changed",
    "Manual restart",
    "Resumed retained prefix",
    "Retained prefix released by memory policy",
    "Perspective camera and a valid viewport are required",
    "The scene contains unsupported material transport",
    "Required GPU ray tracing or accumulation support is unavailable",
    "Another viewport owns the reference accumulator"};
static constexpr std::array ProgressRouteLabels = {"Unavailable", "Inline ray tracing", "Ray-tracing pipeline"};
static constexpr std::array BackendLabels = {"Unknown", "D3D12", "Vulkan"};

static void RequestAction(ViewportRenderRequest& request, ViewportRenderAction action) noexcept
{
	request.RenderAction = action;
	++request.RenderActionSequence;
	++request.Generation;
}

void DrawReferencePathTracerOverlay(const ViewportRenderProgress& progress, ViewportRenderRequest& request) noexcept
{
	const bool unavailable = progress.State == ViewportRenderProgressState::Unavailable;
	const bool paused = progress.State == ViewportRenderProgressState::Paused;
	const bool complete = progress.State == ViewportRenderProgressState::Complete;
	const char* stateLabel = ProgressStateLabels[static_cast<std::size_t>(progress.State)];
	const char* reasonLabel = ProgressReasonLabels[static_cast<std::size_t>(progress.Reason)];
	const float overlayHeight = unavailable ? 118.0f : (complete ? 92.0f : 142.0f);
	ImGui::BeginChild(
	    "##ViewportRenderProgress",
	    ImVec2(330.0f, overlayHeight),
	    ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::TextUnformatted("Reference Path Tracer");

	if (unavailable)
	{
		ImGui::TextUnformatted("Unavailable");
		ImGui::TextWrapped("%s", reasonLabel);
		if (progress.Reason == ViewportRenderProgressReason::SessionCapacity)
		{
			ImGui::Text("Owned by viewport %llu", static_cast<unsigned long long>(progress.OwnerViewportId));
			if (ImGui::Button("Transfer"))
			{
				RequestAction(request, ViewportRenderAction::Transfer);
			}
		}
		else if (ImGui::Button("Use Lit"))
		{
			request.ViewMode = RenderViewMode::Lit;
			++request.Generation;
		}
	}
	else
	{
		const float fraction = progress.TargetWork == 0
		    ? 0.0f
		    : (std::min) (1.0f, static_cast<float>(progress.CompletedWork) / static_cast<float>(progress.TargetWork));
		char sampleLabel[64] = {};
		std::snprintf(
		    sampleLabel,
		    sizeof(sampleLabel),
		    "%llu / %llu SPP",
		    static_cast<unsigned long long>(progress.CompletedWork),
		    static_cast<unsigned long long>(progress.TargetWork));
		ImGui::Text("%s%s", stateLabel, complete ? " - target prefix reached" : "");
		ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), sampleLabel);

		if (progress.SamplesPerSecond > 0.0 && !complete)
		{
			ImGui::Text("%.2f Msamples/s  ETA about %.0f s", progress.SamplesPerSecond / 1000000.0, progress.EstimatedSecondsRemaining);
		}
		else if (!complete)
		{
			ImGui::TextUnformatted("Throughput and ETA calculating");
		}
		if (progress.Reason != ViewportRenderProgressReason::None)
		{
			ImGui::Text("Last event: %s", reasonLabel);
		}

		if (paused)
		{
			if (ImGui::Button("Resume"))
			{
				RequestAction(request, ViewportRenderAction::Resume);
			}
		}
		else if (!complete)
		{
			ImGui::BeginDisabled(!progress.RetentionAvailable);
			if (ImGui::Button("Pause"))
			{
				RequestAction(request, ViewportRenderAction::Pause);
			}
			ImGui::EndDisabled();
			if (!progress.RetentionAvailable && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Retention unavailable for the current GPU memory budget");
			}
		}

		if (!complete || paused)
		{
			ImGui::SameLine();
		}
		if (ImGui::Button("Restart"))
		{
			RequestAction(request, ViewportRenderAction::Restart);
		}
	}

	if (!unavailable || progress.Reason == ViewportRenderProgressReason::SessionCapacity)
	{
		ImGui::SameLine();
	}
	if (ImGui::Button("Details"))
	{
		ImGui::OpenPopup("Reference Path Tracer Details");
	}
	if (ImGui::BeginPopup("Reference Path Tracer Details"))
	{
		ImGui::Text("State: %s", stateLabel);
		ImGui::Text(
		    "Committed prefix: %llu / %llu SPP",
		    static_cast<unsigned long long>(progress.CompletedWork),
		    static_cast<unsigned long long>(progress.TargetWork));
		ImGui::Text("Last event: %s", reasonLabel);
		ImGui::Text("Discarded prefix: %llu SPP", static_cast<unsigned long long>(progress.DiscardedWork));
		ImGui::Text("Route: %s", ProgressRouteLabels[static_cast<std::size_t>(progress.ActiveRoute)]);
		ImGui::Text("Backend: %s", BackendLabels[static_cast<std::size_t>(progress.BackendApi)]);
		ImGui::Separator();
		ImGui::TextUnformatted("Raw: scene-linear HDR accumulation");
		ImGui::TextUnformatted("Display: viewport presentation derived from raw accumulation");
		ImGui::TextDisabled("Evidence/Output: unavailable in this milestone");
		ImGui::EndPopup();
	}

	ImGui::EndChild();
}
