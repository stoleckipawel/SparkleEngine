#include "PCH.h"
#include "ViewMode.h"

#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/RendererCVars.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <imgui.h>

static constexpr const char* kViewModeNames[] = {
    "Lit",
    "GBuffer Diffuse",
    "GBuffer Normal",
    "GBuffer Roughness",
    "GBuffer Metallic",
    "GBuffer Emissive",
    "GBuffer Ambient Occlusion",
    "GBuffer Subsurface Color",
    "GBuffer Subsurface Strength",
    "Direct Diffuse",
    "Direct Specular",
    "Direct Subsurface",
    "Indirect Diffuse",
    "Indirect Specular",
};

static_assert(
    sizeof(kViewModeNames) / sizeof(kViewModeNames[0]) == static_cast<std::size_t>(RenderViewMode::Count),
    "View mode labels must stay in sync with RenderViewMode.");

void ViewMode::BuildUI()
{
	const int maxModeIndex = static_cast<int>(RenderViewMode::Count) - 1;
	int modeIndex = std::clamp(static_cast<int>(CVarRenderViewMode.Get()), 0, maxModeIndex);
	UiUtil::DrawKeyValueRow("Mode", kViewModeNames[modeIndex]);
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo("##ViewMode", &modeIndex, kViewModeNames, static_cast<int>(RenderViewMode::Count)))
	{
		CVarRenderViewMode.Set(static_cast<RenderViewMode>(modeIndex));
	}
}
