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

RayTracingSceneFrameData RenderRayTracingScene::Prepare(const RenderSceneData& sceneData) noexcept
{
	if (m_tlasBuilder == nullptr)
	{
		return {};
	}

	RayTracingSceneFrameData frameData{};
	const std::uint32_t estimatedInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	if (estimatedInstanceCount == 0)
	{
		if (m_tlasBuilder->GetTlas().IsValid())
		{
			frameData.IsAvailable = true;
			frameData.TlasResource = m_tlasBuilder->GetTlas().resource;
			frameData.TlasGpuAddress = m_tlasBuilder->GetTlas().gpuAddress;
		}
		return frameData;
	}

	if (!m_tlasBuilder->Prepare(estimatedInstanceCount))
	{
		return frameData;
	}

	frameData.IsAvailable = true;
	frameData.TlasResource = m_tlasBuilder->GetTlas().resource;
	frameData.TlasGpuAddress = m_tlasBuilder->GetTlas().gpuAddress;
	frameData.EstimatedInstanceCount = estimatedInstanceCount;
	return frameData;
}

void RenderRayTracingScene::Build(RenderCommandContext& cmd, const RenderSceneData& sceneData) noexcept
{
	if (m_blasCache == nullptr || m_tlasBuilder == nullptr)
	{
		return;
	}

	if (sceneData.meshInstances.empty())
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

RhiOwnedResourceHandle RenderRayTracingScene::GetTlasResource() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().resource : RhiOwnedResourceHandle{};
}

RhiGpuVirtualAddress RenderRayTracingScene::GetTlasGpuAddress() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().gpuAddress : 0;
}

std::uint32_t RenderRayTracingScene::GetTlasInstanceCount() const noexcept
{
	return m_tlasBuilder != nullptr ? m_tlasBuilder->GetTlas().instanceCount : 0;
}
