#pragma once

#include "Frame/RayTracingSceneFrameData.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RayTracing/RayTracingSceneDiagnostics.h"
#include "RayTracing/RayTracingTlasBuilder.h"

#include <memory>

class RenderCommandContext;
class RenderHardwareInterface;
struct RenderSceneData;

class RenderRayTracingScene final
{
  public:
	RenderRayTracingScene(RenderHardwareInterface& renderHardwareInterface, const RayTracingCapabilityReport& capabilityReport) noexcept;
	~RenderRayTracingScene() noexcept = default;

	RenderRayTracingScene(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene& operator=(const RenderRayTracingScene&) = delete;
	RenderRayTracingScene(RenderRayTracingScene&&) = delete;
	RenderRayTracingScene& operator=(RenderRayTracingScene&&) = delete;

	RayTracingSceneFrameData Prepare(const RenderSceneData& sceneData) noexcept;
	void Build(RenderCommandContext& cmd, const RenderSceneData& sceneData) noexcept;
	void Clear() noexcept;

	bool IsAvailable() const noexcept { return m_capabilityReport.SupportsRayTracing; }
	bool HasValidTlas() const noexcept { return m_tlasBuilder != nullptr && m_tlasBuilder->GetTlas().IsValid(); }
	NativeResourceHandle GetTlasResource() const noexcept;
	RhiGpuVirtualAddress GetTlasGpuAddress() const noexcept;
	std::uint32_t GetTlasInstanceCount() const noexcept;
	const RayTracingCapabilityReport& GetCapabilities() const noexcept { return m_capabilityReport; }

  private:
	RayTracingCapabilityReport m_capabilityReport = {};
	std::unique_ptr<RayTracingBlasCache> m_blasCache;
	std::unique_ptr<RayTracingTlasBuilder> m_tlasBuilder;
	RayTracingSceneDiagnostics m_diagnostics;
};
