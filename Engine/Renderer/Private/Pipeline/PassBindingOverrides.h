#pragma once

#include "RHI/Public/Interop/RenderHardwareInterface.h"

#include <cstdint>
#include <string>
#include <vector>

enum class PassBindingOverrideType : std::uint8_t
{
	ConstantBufferView,
	ShaderResourceView,
	UnorderedAccessView,
	DescriptorTable,
	RootConstants,
};

enum class DescriptorTableOverrideKind : std::uint8_t
{
	GpuDescriptor = 0,
	LogicalTable = 1,
};

struct PassBindingOverride
{
	std::string Name;
	PassBindingOverrideType Type = PassBindingOverrideType::DescriptorTable;
	RhiGpuVirtualAddress GpuAddress = 0;
	RhiGpuDescriptorHandle DescriptorTable = {};
	RhiDescriptorTableHandle LogicalDescriptorTable = {};
	DescriptorTableOverrideKind DescriptorTableKind = DescriptorTableOverrideKind::GpuDescriptor;
	const void* ConstantsData = nullptr;
	std::uint32_t ConstantCount = 0;
};

class PassBindingOverrides final
{
  public:
	void SetConstantBufferView(const char* name, RhiGpuVirtualAddress gpuAddress);
	void SetShaderResourceView(const char* name, RhiGpuVirtualAddress gpuAddress);
	void SetUnorderedAccessView(const char* name, RhiGpuVirtualAddress gpuAddress);
	void SetDescriptorTable(const char* name, RhiGpuDescriptorHandle descriptorTable);
	void SetDescriptorTable(const char* name, RhiDescriptorTableHandle descriptorTable);
	void SetRootConstants(const char* name, const void* data, std::uint32_t constantCount);

	const PassBindingOverride* Find(const char* name, PassBindingOverrideType type) const noexcept;

  private:
	std::vector<PassBindingOverride> m_overrides;
};