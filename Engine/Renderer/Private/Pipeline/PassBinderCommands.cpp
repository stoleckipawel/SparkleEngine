#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "Commands/RenderCommandContext.h"

#include <cassert>

bool PassBinder::TryBindDescriptorTableOverride(
    RenderCommandContext& cmd,
    const CompiledBinding& compiledBinding,
    const PassBindingOverrides* overrides,
    bool isCompute)
{
	const PassBindingOverride* bindingOverride =
	    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::DescriptorTable) : nullptr;
	if (bindingOverride == nullptr)
	{
		return false;
	}

	if (bindingOverride->DescriptorTableKind == DescriptorTableOverrideKind::LogicalTable)
	{
		BindDescriptorTable(cmd, compiledBinding, bindingOverride->LogicalDescriptorTable, isCompute);
	}
	else
	{
		BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
	}
	return true;
}

void PassBinder::BindGpuAddress(
    RenderCommandContext& cmd,
    const CompiledBinding& compiledBinding,
    RhiGpuVirtualAddress gpuAddress,
    bool isCompute)
{
	if (isCompute)
	{
		switch (compiledBinding.Type)
		{
			case CompiledBindingType::ConstantBuffer:
				cmd.BindComputeConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
				return;
			case CompiledBindingType::ReadOnlyAddress:
				cmd.BindComputeShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
				return;
			case CompiledBindingType::ReadWriteAddress:
				cmd.BindComputeUnorderedAccessAddress(compiledBinding.BindingIndex, gpuAddress);
				return;
			default:
				assert(false);
				return;
		}
	}

	switch (compiledBinding.Type)
	{
		case CompiledBindingType::ConstantBuffer:
			cmd.BindConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
			return;
		case CompiledBindingType::ReadOnlyAddress:
			cmd.BindShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
			return;
		case CompiledBindingType::ReadWriteAddress:
			cmd.BindUnorderedAccessAddress(compiledBinding.BindingIndex, gpuAddress);
			return;
		default:
			assert(false);
			return;
	}
}

void PassBinder::BindDescriptorTable(
    RenderCommandContext& cmd,
    const CompiledBinding& compiledBinding,
    RhiGpuDescriptorHandle descriptorTable,
    bool isCompute)
{
	if (isCompute)
	{
		cmd.BindComputeDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}

	cmd.BindDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
}

void PassBinder::BindDescriptorTable(
    RenderCommandContext& cmd,
    const CompiledBinding& compiledBinding,
    RhiDescriptorTableBinding descriptorTable,
    bool isCompute)
{
	if (isCompute)
	{
		cmd.BindComputeDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}

	cmd.BindDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
}

void PassBinder::BindPushConstants(
    RenderCommandContext& cmd,
    const CompiledBinding& compiledBinding,
    const void* data,
    std::uint32_t constantCount,
    bool isCompute)
{
	assert(data != nullptr);
	assert(constantCount > 0);

	if (isCompute)
	{
		cmd.SetComputePushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
		return;
	}

	cmd.SetPushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
}
