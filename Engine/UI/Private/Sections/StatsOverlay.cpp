#include "PCH.h"
#include "StatsOverlay.h"

#include "Timer.h"
#include "Util/UiUtil.h"

#include <cstdio>
#include <imgui.h>

StatsOverlay::StatsOverlay(Timer& timer) noexcept : m_timer(timer) {}

void StatsOverlay::BuildUI()
{
	ImGuiIO& io = ImGui::GetIO();
	char fpsText[32] = {};
	char frameTimeText[32] = {};
	char frameIndexText[32] = {};
	std::snprintf(fpsText, sizeof(fpsText), "%.1f", io.Framerate);
	std::snprintf(frameTimeText, sizeof(frameTimeText), "%.2f ms", io.DeltaTime * 1000.0f);
	std::snprintf(frameIndexText, sizeof(frameIndexText), "%llu", static_cast<unsigned long long>(m_timer.GetFrameCount()));

	UiUtil::DrawKeyValueRow("FPS", fpsText);
	UiUtil::DrawKeyValueRow("Frame", frameIndexText);
	UiUtil::DrawKeyValueRow("Delta", frameTimeText);
}
