#include "../PCH.h"
#include "Pipeline/PassBindingOverrides.h"

void PassBindingOverrides::SetConstantBufferView(const char* name, RhiGpuVirtualAddress gpuAddress)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::ConstantBufferView,
	        .GpuAddress = gpuAddress});
}

void PassBindingOverrides::SetShaderResourceView(const char* name, RhiGpuVirtualAddress gpuAddress)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::ShaderResourceView,
	        .GpuAddress = gpuAddress});
}

void PassBindingOverrides::SetUnorderedAccessView(const char* name, RhiGpuVirtualAddress gpuAddress)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::UnorderedAccessView,
	        .GpuAddress = gpuAddress});
}

void PassBindingOverrides::SetDescriptorTable(const char* name, RhiGpuDescriptorHandle descriptorTable)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::DescriptorTable,
	        .DescriptorTable = descriptorTable,
	        .DescriptorTableKind = DescriptorTableOverrideKind::GpuDescriptor});
}

void PassBindingOverrides::SetDescriptorTable(const char* name, RhiDescriptorTableBinding descriptorTable)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::DescriptorTable,
	        .LogicalDescriptorTable = descriptorTable,
	        .DescriptorTableKind = DescriptorTableOverrideKind::LogicalTable});
}

void PassBindingOverrides::SetPushConstants(const char* name, const void* data, std::uint32_t constantCount)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::PushConstants,
	        .ConstantsData = data,
	        .ConstantCount = constantCount});
}

const PassBindingOverride* PassBindingOverrides::Find(const char* name, PassBindingOverrideType type) const noexcept
{
	if (name == nullptr)
	{
		return nullptr;
	}

	for (const PassBindingOverride& bindingOverride : m_overrides)
	{
		if (bindingOverride.Type == type && bindingOverride.Name == name)
		{
			return &bindingOverride;
		}
	}

	return nullptr;
}
