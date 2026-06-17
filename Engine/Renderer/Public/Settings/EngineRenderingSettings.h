#pragma once

#include "Core/Public/Config/ConfigBackedSettings.h"
#include "RendererAPI.h"

#include <string>

enum class EngineRayTracingTopLevelMode
{
	ClassicTlas,
	PartitionedTlas,
};

enum class EnginePtlasUpdatePath
{
	CpuPack,
	GpuLogicalDirtyCpuNativePack,
	FullGpuNativePack,
};

struct EngineRenderingSettingsState final
{
	bool VSync = true;
	bool PreferHighPerformanceAdapter = true;
	bool MeshAutoBatching = true;
	EngineRayTracingTopLevelMode RayTracingTopLevelMode = EngineRayTracingTopLevelMode::ClassicTlas;
	EnginePtlasUpdatePath PtlasUpdatePath = EnginePtlasUpdatePath::CpuPack;
};

class SPARKLE_RENDERER_API EngineRenderingSettingsSection final : public ConfigBackedSettingsSection<EngineRenderingSettingsState>
{
  public:
	EngineRenderingSettingsSection();

	void SetVSync(bool enabled);
	void SetPreferHighPerformanceAdapter(bool enabled);
	void SetMeshAutoBatching(bool enabled);
	void SetRayTracingTopLevelMode(EngineRayTracingTopLevelMode mode);
	void SetPtlasUpdatePath(EnginePtlasUpdatePath path);

  private:
	EngineRenderingSettingsState CaptureRuntimeState() const noexcept override;
	void ApplyStateToRuntime(const EngineRenderingSettingsState& state) const noexcept override;
	void ReadConfigValue(EngineRenderingSettingsState& state, std::string_view key, std::string_view value) const override;
	std::vector<std::pair<std::string, std::string>> BuildConfigValues(const EngineRenderingSettingsState& state) const override;
	bool ComputePendingRestart(
	    const EngineRenderingSettingsState& baseline,
	    const EngineRenderingSettingsState& current) const noexcept override;
	std::string DescribePendingRestart(
	    const EngineRenderingSettingsState& baseline,
	    const EngineRenderingSettingsState& current) const override;
};

SPARKLE_RENDERER_API void ApplyPersistedEngineRenderingSettingsToCVars() noexcept;
