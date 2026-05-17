#include "PCH.h"

#include "RHI/Public/Bindings/RenderBindingSet.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <utility>

RenderBindingSet::RenderBindingSet(RenderHardwareInterface& renderHardwareInterface, const RenderBindingSetDesc& desc) noexcept :
	m_renderHardwareInterface(&renderHardwareInterface),
	m_tableHandle(renderHardwareInterface.AllocateDescriptorTable(desc.DescriptorType, desc.DescriptorCount)),
	m_descriptorCount(desc.DescriptorCount)
{
}

RenderBindingSet::~RenderBindingSet() noexcept
{
	Reset();
}

RenderBindingSet::RenderBindingSet(RenderBindingSet&& other) noexcept :
	m_renderHardwareInterface(std::exchange(other.m_renderHardwareInterface, nullptr)),
	m_tableHandle(std::exchange(other.m_tableHandle, RhiDescriptorTableHandle{})),
	m_descriptorCount(std::exchange(other.m_descriptorCount, 0))
{
}

RenderBindingSet& RenderBindingSet::operator=(RenderBindingSet&& other) noexcept
{
	if (this != &other)
	{
		Reset();
		m_renderHardwareInterface = std::exchange(other.m_renderHardwareInterface, nullptr);
		m_tableHandle = std::exchange(other.m_tableHandle, RhiDescriptorTableHandle{});
		m_descriptorCount = std::exchange(other.m_descriptorCount, 0);
	}

	return *this;
}

RhiCpuDescriptorHandle RenderBindingSet::GetCpuDescriptorHandle(std::uint32_t descriptorIndex) const noexcept
{
	return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetDescriptorTableCpuHandle(m_tableHandle, descriptorIndex)
	                                        : RhiCpuDescriptorHandle{};
}

RhiDescriptorTableBinding RenderBindingSet::GetTableBinding(std::uint32_t descriptorIndex) const noexcept
{
	return RhiDescriptorTableBinding{.Table = m_tableHandle, .DescriptorIndex = descriptorIndex};
}

void RenderBindingSet::Reset() noexcept
{
	if (m_renderHardwareInterface != nullptr && m_tableHandle)
	{
		m_renderHardwareInterface->ReleaseDescriptorTable(m_tableHandle);
	}

	m_renderHardwareInterface = nullptr;
	m_tableHandle = {};
	m_descriptorCount = 0;
}