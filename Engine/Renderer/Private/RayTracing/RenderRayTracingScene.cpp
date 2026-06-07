#include "PCH.h"

#include "RayTracing/RenderRayTracingScene.h"

#include "Commands/RenderCommandContext.h"
#include "SceneData/RenderSceneData.h"

RenderRayTracingScene::RenderRayTracingScene(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_capabilityReport(capabilityReport)
{
	if (!m_capabilityReport.SupportsRayTracing)
	{
		return;
	}

	m_blasCache = std::make_unique<RayTracingBlasCache>(renderHardwareInterface);
	m_tlasBuilder = std::make_unique<RayTracingTlasBuilder>(renderHardwareInterface);
}

void RenderRayTracingScene::Update(RenderCommandContext& cmd, const RenderSceneData& sceneData) noexcept
{
	if (m_blasCache == nullptr || m_tlasBuilder == nullptr)
	{
		return;
	}

	m_blasCache->BeginFrame();
	const RayTracingTlasBuilder::BuildStats tlasStats = m_tlasBuilder->Build(cmd, sceneData, *m_blasCache);
	const RayTracingBlasCache::BuildStats blasStats = m_blasCache->EndFrame();
	m_diagnostics.LogSceneUpdate(m_capabilityReport, blasStats, tlasStats);
}

void RenderRayTracingScene::Clear() noexcept
{
	if (m_tlasBuilder != nullptr)
	{
		m_tlasBuilder->Clear();
	}
	
	if (m_blasCache != nullptr)
	{
		m_blasCache->Clear();
	}
}

NativeResourceHandle RenderRayTracingScene::GetTlasResource() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().resource : NativeResourceHandle{};
}

RhiGpuVirtualAddress RenderRayTracingScene::GetTlasGpuAddress() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().gpuAddress : 0;
}

std::uint32_t RenderRayTracingScene::GetTlasInstanceCount() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().instanceCount : 0;
}
