#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "Commands/RenderCommandContext.h"

void PassBinder::BindDescriptorTableOverride(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    const PassBindingOverride& bindingOverride,
    bool isCompute)
{
	if (bindingOverride.DescriptorTableKind == DescriptorTableOverrideKind::LogicalTable)
	{
		BindDescriptorTable(commandContext, compiledBinding, bindingOverride.LogicalDescriptorTable, isCompute);
	}
	else
	{
		BindDescriptorTable(commandContext, compiledBinding, bindingOverride.DescriptorTable, isCompute);
	}
}

void PassBinder::BindGpuAddress(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    RhiGpuVirtualAddress gpuAddress,
    bool isCompute)
{
	Require(gpuAddress != 0, "Pass address binding resolved to a null GPU address.");
	switch (compiledBinding.Type)
	{
		case CompiledBindingType::ConstantBuffer:
			if (isCompute)
			{
				commandContext.BindComputeConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
			}
			else
			{
				commandContext.BindConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
			}
			return;
		case CompiledBindingType::ReadOnlyAddress:
			if (isCompute)
			{
				commandContext.BindComputeShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			else
			{
				commandContext.BindShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			return;
		case CompiledBindingType::ReadWriteAddress:
			if (isCompute)
			{
				commandContext.BindComputeUnorderedAccessAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			else
			{
				commandContext.BindUnorderedAccessAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			return;
		default:
			Require(false, "Pass address binding has an unsupported type.");
			return;
	}
}

void PassBinder::BindDescriptorTable(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    RhiGpuDescriptorHandle descriptorTable,
    bool isCompute)
{
	Require(static_cast<bool>(descriptorTable), "Pass resource binding resolved to a null GPU descriptor handle.");
	if (isCompute)
	{
		commandContext.BindComputeDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}

	commandContext.BindDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
}

void PassBinder::BindDescriptorTable(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    RhiDescriptorTableBinding descriptorTable,
    bool isCompute)
{
	Require(static_cast<bool>(descriptorTable), "Pass resource binding resolved to an invalid logical descriptor table.");
	if (isCompute)
	{
		commandContext.BindComputeDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}

	commandContext.BindDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
}

void PassBinder::BindPushConstants(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    const void* data,
    std::uint32_t constantCount,
    bool isCompute)
{
	Require(data != nullptr, "Push-constant binding has no data.");
	Require(constantCount > 0, "Push-constant binding has no values.");

	if (isCompute)
	{
		commandContext.SetComputePushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
		return;
	}

	commandContext.SetPushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
}
