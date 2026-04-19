#pragma once

#include "../EditorAPI.h"
#include "Core/Public/Diagnostics/LiveProfiler.h"

class SPARKLE_EDITOR_API ProfilerPanel final
{
  public:
	ProfilerPanel() noexcept = default;
	~ProfilerPanel() = default;

	ProfilerPanel(const ProfilerPanel&) = delete;
	ProfilerPanel(ProfilerPanel&&) = delete;
	ProfilerPanel& operator=(const ProfilerPanel&) = delete;
	ProfilerPanel& operator=(ProfilerPanel&&) = delete;

	void SetTopInset(float topInsetPixels) noexcept { m_topInsetPixels = topInsetPixels; }
	void BuildUI(bool disableInteraction = false);

  private:
	void RenderCpuTab() const;
	void RenderGpuTab() const;
	static void RenderNodeRow(const Engine::Diagnostics::ProfilerSnapshotNode& node);
	static void RenderToolbar(Engine::Diagnostics::LiveProfiler& profiler) noexcept;

	Engine::Diagnostics::ProfilerSnapshot m_snapshot;
	float m_topInsetPixels = 0.0f;
	float m_widthPixels = 520.0f;
	float m_heightPixels = 420.0f;
	bool m_isVisible = true;
};
