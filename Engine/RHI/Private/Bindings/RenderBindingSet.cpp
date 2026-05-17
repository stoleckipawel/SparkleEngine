#include "PCH.h"

#include "RHI/Public/Bindings/RenderBindingSet.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Validation/RhiValidation.h"

#include <utility>

RenderBindingSet::RenderBindingSet(RenderHardwareInterface& renderHardwareInterface, const RenderBindingSetDesc& desc) noexcept :
	m_renderHardwareInterface(&renderHardwareInterface)
{
	if (!RhiValidation::ValidateBindingSetDesc(renderHardwareInterface.GetCapabilities(), desc, "RHI.RenderBindingSet"))
	{
		m_renderHardwareInterface = nullptr;
		return;
	}

	m_tableHandle = renderHardwareInterface.AllocateDescriptorTable(desc.DescriptorType, desc.DescriptorCount);
	m_descriptorCount = desc.DescriptorCount;
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
	if (!RhiValidation::ValidateBindingSetDescriptorIndex(descriptorIndex, m_descriptorCount, "RHI.RenderBindingSet"))
	{
		return {};
	}

	return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetDescriptorTableCpuHandle(m_tableHandle, descriptorIndex)
	                                        : RhiCpuDescriptorHandle{};
}

RhiDescriptorTableBinding RenderBindingSet::GetTableBinding(std::uint32_t descriptorIndex) const noexcept
{
	if (!RhiValidation::ValidateBindingSetDescriptorIndex(descriptorIndex, m_descriptorCount, "RHI.RenderBindingSet"))
	{
		return {};
	}

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