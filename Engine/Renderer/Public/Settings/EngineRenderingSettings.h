#pragma once

#include "Core/Public/Config/ConfigBackedSettings.h"
#include "RHI/Public/Presentation/RhiPresentationDefaults.h"
#include "RHI/Public/Resources/RenderLightingLimits.h"
#include "RendererAPI.h"

#include <cstdint>
#include <string>

enum class EnginePtlasPartitionTopology
{
	XZ2D,
	XYZ3D,
};

enum class EnginePtlasPartitionUpdateMode
{
	AlwaysUpdatePartition,
	AlwaysMoveDynamicToGlobal,
	UpdatePartitionNearbyMoveToGlobalOtherwise,
};

struct EngineRenderingSettingsState final
{
	bool VSync = true;
	PixelFormat BackBufferFormat = RhiPresentationDefaults::BackBufferFormat;
	bool PreferHighPerformanceAdapter = true;
	std::uint32_t MaxDirectionalLights = static_cast<std::uint32_t>(RenderLightingLimits::MaxDirectionalLights);
	std::uint32_t MaxPointLights = static_cast<std::uint32_t>(RenderLightingLimits::MaxPointLights);
	std::uint32_t MaxSpotLights = static_cast<std::uint32_t>(RenderLightingLimits::MaxSpotLights);
	bool MeshAutoBatching = true;
	bool RefitTlas = true;
	bool PtlasActive = false;
	std::uint32_t PtlasPartitionsPerAxis = 8;
	EnginePtlasPartitionTopology PtlasPartitionTopology = EnginePtlasPartitionTopology::XYZ3D;
	EnginePtlasPartitionUpdateMode PtlasPartitionUpdateMode = EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition;
	bool PtlasMarkAllDynamicInPartition = false;
	float PtlasModeChangeDistance = 100.0f;
};

class SPARKLE_RENDERER_API EngineRenderingSettingsSection final : public ConfigBackedSettingsSection<EngineRenderingSettingsState>
{
  public:
	EngineRenderingSettingsSection();

	void SetVSync(bool enabled);
	void SetBackBufferFormat(PixelFormat format);
	void SetPreferHighPerformanceAdapter(bool enabled);
	void SetMaxDirectionalLights(std::uint32_t count);
	void SetMaxPointLights(std::uint32_t count);
	void SetMaxSpotLights(std::uint32_t count);
	void SetMeshAutoBatching(bool enabled);
	void SetRefitTlas(bool enabled);
	void SetPtlasActive(bool active);
	void SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis);
	void SetPtlasPartitionTopology(EnginePtlasPartitionTopology topology);
	void SetPtlasPartitionUpdateMode(EnginePtlasPartitionUpdateMode mode);
	void SetPtlasMarkAllDynamicInPartition(bool enabled);
	void SetPtlasModeChangeDistance(float distance);

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
