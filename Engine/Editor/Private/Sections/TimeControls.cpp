#include "PCH.h"
#include "TimeControls.h"

#include "Timer.h"
#include "Util/UiUtil.h"

#include <imgui.h>

TimeControls::TimeControls(Timer& timer) noexcept : m_timer(timer) {}

void TimeControls::BuildUI()
{
	const bool paused = m_timer.IsPaused();
	UiUtil::DrawKeyValueRow("State", paused ? "Paused" : "Running");

	float timeScale = static_cast<float>(m_timer.GetTimeScale());
	if (UiUtil::EditFloatSliderWithInput("Scale", timeScale, 0.0f, 4.0f, "%.2fx", "%.2f"))
	{
		m_timer.SetTimeScale(static_cast<double>(timeScale));
	}

	if (ImGui::Button(paused ? "Resume" : "Pause", ImVec2(-1.0f, 0.0f)))
	{
		if (paused)
			m_timer.Resume();
		else
			m_timer.Pause();
	}
}
