#include "PCH.h"

#include "RHI/Public/Bindings/RenderBindingSet.h"

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Descriptors/RhiDescriptorService.h"
#include "Validation/RhiContract.h"

#include <utility>

RenderBindingSet::RenderBindingSet(
    const RhiCapabilities& capabilities,
    RhiDescriptorService& descriptorService,
    const RenderBindingSetDesc& desc) noexcept :
	m_descriptorService(&descriptorService)
{
	if (!RhiContract::IsBindingSetDescUsable(capabilities, desc))
	{
		m_descriptorService = nullptr;
		return;
	}

	m_tableHandle = descriptorService.AllocateDescriptorTable(desc.DescriptorType, desc.DescriptorCount);
	m_descriptorCount = desc.DescriptorCount;
}

RenderBindingSet::~RenderBindingSet() noexcept
{
	Reset();
}

RenderBindingSet::RenderBindingSet(RenderBindingSet&& other) noexcept :
	m_descriptorService(std::exchange(other.m_descriptorService, nullptr)),
	m_tableHandle(std::exchange(other.m_tableHandle, RhiDescriptorTableHandle{})),
	m_descriptorCount(std::exchange(other.m_descriptorCount, 0))
{
}

RenderBindingSet& RenderBindingSet::operator=(RenderBindingSet&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_descriptorService = std::exchange(other.m_descriptorService, nullptr);
		m_tableHandle = std::exchange(other.m_tableHandle, RhiDescriptorTableHandle{});
		m_descriptorCount = std::exchange(other.m_descriptorCount, 0);
	}

	return *this;
}

RhiCpuDescriptorHandle RenderBindingSet::GetCpuDescriptorHandle(std::uint32_t descriptorIndex) const noexcept
{
	if (!RhiContract::IsBindingSetDescriptorIndexValid(descriptorIndex, m_descriptorCount))
	{
		return {};
	}

	return m_descriptorService != nullptr ? m_descriptorService->GetDescriptorTableCpuHandle(m_tableHandle, descriptorIndex) :
	                                       RhiCpuDescriptorHandle{};
}

RhiDescriptorTableBinding RenderBindingSet::GetTableBinding(std::uint32_t descriptorIndex) const noexcept
{
	if (!RhiContract::IsBindingSetDescriptorIndexValid(descriptorIndex, m_descriptorCount))
	{
		return {};
	}

	return RhiDescriptorTableBinding{.Table = m_tableHandle, .DescriptorIndex = descriptorIndex};
}

void RenderBindingSet::Reset() noexcept
{
	if (m_descriptorService != nullptr && m_tableHandle)
	{
		m_descriptorService->ReleaseDescriptorTable(m_tableHandle);
	}

	m_descriptorService = nullptr;
	m_tableHandle = {};
	m_descriptorCount = 0;
}
