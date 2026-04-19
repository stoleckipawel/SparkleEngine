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

	void BuildEmbeddedUI(bool disableInteraction = false);

  private:
	enum class SortMode : int
	{
		Hierarchy = 0,
		AlphabeticalAsc = 1,
		AlphabeticalDesc = 2,
		InclusiveDescending = 3,
		ExclusiveDescending = 4,
		MaxDescending = 5,
		CallsDescending = 6,
	};

	void RenderCpuTab() const;
	void RenderGpuTab() const;
	void RenderGroupedNodes(const std::vector<Engine::Diagnostics::ProfilerSnapshotNode>& nodes) const;
	void RenderTableRows(
	    const std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& nodes,
	    int depth) const;
	void RenderNodeRow(const Engine::Diagnostics::ProfilerSnapshotNode& node, int depth) const;
	void RenderModuleCharts(
	    const std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& bucket,
	    std::string_view moduleName) const;
	void BeginProfilerTable(const char* id) const;
	void RenderToolbar(Engine::Diagnostics::LiveProfiler& profiler) noexcept;
	void SortBucket(std::vector<const Engine::Diagnostics::ProfilerSnapshotNode*>& bucket) const;

	Engine::Diagnostics::ProfilerSnapshot m_snapshot;
	SortMode m_sortMode = SortMode::Hierarchy;
	bool m_showCharts = true;
};
