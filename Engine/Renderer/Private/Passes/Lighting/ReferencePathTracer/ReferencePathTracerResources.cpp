#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"

#include "Diagnostics/RendererMemoryMonitor.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "RHI/Public/Resources/RhiResourceService.h"

#include <algorithm>

ReferencePathTracerResources::ReferencePathTracerResources(
    RenderDeviceServices& deviceServices,
    RendererMemoryMonitor& memoryMonitor) noexcept :
    m_deviceServices(deviceServices),
    m_memoryMonitor(memoryMonitor)
{
}

ReferencePathTracerResources::~ReferencePathTracerResources() noexcept
{
	Release();
}

void ReferencePathTracerResources::ReserveGraphResources(FrameGraphBuilder& builder, RenderViewportExtent extent)
{
	m_graphResources.WorkingMean = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ReferencePathTracer.WorkingMean",
	        extent.Width,
	        extent.Height,
	        PixelFormat::R32G32B32A32_Float));
	m_graphResources.WorkingM2 = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("ReferencePathTracer.WorkingM2", extent.Width, extent.Height, PixelFormat::R32G32B32A32_Float));
	m_graphResources.CommittedMean = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ReferencePathTracer.CommittedMean",
	        extent.Width,
	        extent.Height,
	        PixelFormat::R32G32B32A32_Float));
	m_graphResources.CommittedM2 = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ReferencePathTracer.CommittedM2",
	        extent.Width,
	        extent.Height,
	        PixelFormat::R32G32B32A32_Float));
}

void ReferencePathTracerResources::Allocate(RenderViewportExtent extent)
{
	if (m_allocation && m_allocation.Extent == extent)
	{
		return;
	}

	Release();

	RhiResourceService& resources = m_deviceServices.GetRenderHardwareInterface().GetResourceService();
	const RhiTextureResourceDesc desc{
	    .Width = extent.Width,
	    .Height = extent.Height,
	    .Format = PixelFormat::R32G32B32A32_Float,
	    .AllowUnorderedAccess = true};

	m_allocation.WorkingMean = resources.CreateTextureResource(
	    desc,
	    ResourceState::Undefined,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"ReferencePathTracer.WorkingMean");
	m_allocation.WorkingM2 = resources.CreateTextureResource(
	    desc,
	    ResourceState::Undefined,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"ReferencePathTracer.WorkingM2");
	m_allocation.CommittedMean = resources.CreateTextureResource(
	    desc,
	    ResourceState::Undefined,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"ReferencePathTracer.CommittedMean");
	m_allocation.CommittedM2 = resources.CreateTextureResource(
	    desc,
	    ResourceState::Undefined,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"ReferencePathTracer.CommittedM2");

	m_allocation.Extent = extent;
	m_allocation.Bytes = resources.GetTextureAllocationInfo(desc).SizeInBytes * 4u;
}

void ReferencePathTracerResources::Release() noexcept
{
	RhiResourceService& resources = m_deviceServices.GetRenderHardwareInterface().GetResourceService();
	if (m_allocation.WorkingMean)
	{
		resources.ReleaseOwnedResource(m_allocation.WorkingMean);
	}
	if (m_allocation.WorkingM2)
	{
		resources.ReleaseOwnedResource(m_allocation.WorkingM2);
	}
	if (m_allocation.CommittedMean)
	{
		resources.ReleaseOwnedResource(m_allocation.CommittedMean);
	}
	if (m_allocation.CommittedM2)
	{
		resources.ReleaseOwnedResource(m_allocation.CommittedM2);
	}

	m_allocation = {};
}

bool ReferencePathTracerResources::Bind(FrameGraphBuilder& builder) const noexcept
{
	if (!m_allocation)
	{
		return false;
	}

	builder.BindPersistentTexture(m_graphResources.WorkingMean, m_allocation.WorkingMean, m_allocation.BoundaryState);
	builder.BindPersistentTexture(m_graphResources.WorkingM2, m_allocation.WorkingM2, m_allocation.BoundaryState);
	builder.BindPersistentTexture(m_graphResources.CommittedMean, m_allocation.CommittedMean, m_allocation.BoundaryState);
	builder.BindPersistentTexture(m_graphResources.CommittedM2, m_allocation.CommittedM2, m_allocation.BoundaryState);

	return true;
}

void ReferencePathTracerResources::RecordSubmission() noexcept
{
	m_allocation.BoundaryState = ResourceState::Common;
}

bool ReferencePathTracerResources::CanRetain() const noexcept
{
	const RendererMemoryDiagnosticsSnapshot& memory = m_memoryMonitor.GetLatestSnapshot();
	if (!memory.Available || !memory.MemoryUsage.HasBudgetData)
	{
		return false;
	}

	const std::uint64_t retentionBudget = (std::min) (std::uint64_t{2} * 1024u * 1024u * 1024u, memory.MemoryUsage.TotalBudgetBytes / 4u);

	return m_allocation.Bytes <= retentionBudget;
}

bool ReferencePathTracerResources::IsAllocated() const noexcept
{
	return static_cast<bool>(m_allocation);
}
