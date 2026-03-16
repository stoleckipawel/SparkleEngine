#include "PCH.h"
#include "ViewMode.h"

#include "Util/UiUtil.h"

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

void ViewMode::BuildUI()
{
	int modeIndex = static_cast<int>(m_mode);
    UiUtil::DrawKeyValueRow("Mode", kViewModeNames[modeIndex]);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##ViewMode", &modeIndex, kViewModeNames, static_cast<int>(sizeof(kViewModeNames) / sizeof(kViewModeNames[0]))))
	{
		m_mode = static_cast<Type>(modeIndex);
	}
}
