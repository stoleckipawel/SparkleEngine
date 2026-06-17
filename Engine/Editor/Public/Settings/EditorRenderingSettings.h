#pragma once

#include "EditorSettingsService.h"

#include <string>

enum class EditorRayTracingTopLevelMode
{
	ClassicTlas,
	PartitionedTlas,
};

enum class EditorPtlasUpdatePath
{
	CpuPack,
	GpuLogicalDirtyCpuNativePack,
	FullGpuNativePack,
};

struct EditorRenderingSettingsState final
{
	bool VSync = true;
	bool PreferHighPerformanceAdapter = true;
	bool MeshAutoBatching = true;
	EditorRayTracingTopLevelMode RayTracingTopLevelMode = EditorRayTracingTopLevelMode::ClassicTlas;
	EditorPtlasUpdatePath PtlasUpdatePath = EditorPtlasUpdatePath::CpuPack;
};

class SPARKLE_EDITOR_API EditorRenderingSettingsSection final : public EditorConfigBackedSettingsSection<EditorRenderingSettingsState>
{
  public:
	EditorRenderingSettingsSection();

	void SetVSync(bool enabled);
	void SetPreferHighPerformanceAdapter(bool enabled);
	void SetMeshAutoBatching(bool enabled);
	void SetRayTracingTopLevelMode(EditorRayTracingTopLevelMode mode);
	void SetPtlasUpdatePath(EditorPtlasUpdatePath path);

  private:
	EditorRenderingSettingsState CaptureRuntimeState() const noexcept override;
	void ApplyStateToRuntime(const EditorRenderingSettingsState& state) const noexcept override;
	void ReadConfigValue(EditorRenderingSettingsState& state, std::string_view key, std::string_view value) const override;
	std::vector<std::pair<std::string, std::string>> BuildConfigValues(const EditorRenderingSettingsState& state) const override;
	bool ComputePendingRestart(
	    const EditorRenderingSettingsState& baseline,
	    const EditorRenderingSettingsState& current) const noexcept override;
	std::string DescribePendingRestart(
	    const EditorRenderingSettingsState& baseline,
	    const EditorRenderingSettingsState& current) const override;
};
