#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "Commands/RenderCommandContext.h"

void PassBinder::BindDescriptorTableOverride(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    const PassBindingOverride& bindingOverride,
    BindingDomain domain)
{
	if (bindingOverride.DescriptorTableKind == DescriptorTableOverrideKind::LogicalTable)
	{
		BindDescriptorTable(commandContext, compiledBinding, bindingOverride.LogicalDescriptorTable, domain);
	}
	else
	{
		BindDescriptorTable(commandContext, compiledBinding, bindingOverride.DescriptorTable, domain);
	}
}

void PassBinder::BindGpuAddress(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    RhiGpuVirtualAddress gpuAddress,
    BindingDomain domain)
{
	Require(gpuAddress != 0, "Pass address binding resolved to a null GPU address.");
	switch (compiledBinding.Type)
	{
		case CompiledBindingType::ConstantBuffer:
			if (domain == BindingDomain::Compute)
			{
				commandContext.BindComputeConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
			}
			else if (domain == BindingDomain::RayTracing)
			{
				commandContext.BindRayTracingConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
			}
			else
			{
				commandContext.BindConstantBuffer(compiledBinding.BindingIndex, gpuAddress);
			}
			return;
		case CompiledBindingType::ReadOnlyAddress:
			if (domain == BindingDomain::Compute)
			{
				commandContext.BindComputeShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			else if (domain == BindingDomain::RayTracing)
			{
				commandContext.BindRayTracingShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			else
			{
				commandContext.BindShaderResourceAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			return;
		case CompiledBindingType::ReadWriteAddress:
			if (domain == BindingDomain::Compute)
			{
				commandContext.BindComputeUnorderedAccessAddress(compiledBinding.BindingIndex, gpuAddress);
			}
			else if (domain == BindingDomain::RayTracing)
			{
				commandContext.BindRayTracingUnorderedAccessAddress(compiledBinding.BindingIndex, gpuAddress);
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
    BindingDomain domain)
{
	Require(static_cast<bool>(descriptorTable), "Pass resource binding resolved to a null GPU descriptor handle.");
	if (domain == BindingDomain::Compute)
	{
		commandContext.BindComputeDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}
	if (domain == BindingDomain::RayTracing)
	{
		commandContext.BindRayTracingDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}

	commandContext.BindDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
}

void PassBinder::BindDescriptorTable(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    RhiDescriptorTableBinding descriptorTable,
    BindingDomain domain)
{
	Require(static_cast<bool>(descriptorTable), "Pass resource binding resolved to an invalid logical descriptor table.");
	if (domain == BindingDomain::Compute)
	{
		commandContext.BindComputeDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}
	if (domain == BindingDomain::RayTracing)
	{
		commandContext.BindRayTracingDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
		return;
	}

	commandContext.BindDescriptorTable(compiledBinding.BindingIndex, descriptorTable);
}

void PassBinder::BindPushConstants(
    RenderCommandContext& commandContext,
    const CompiledBinding& compiledBinding,
    const void* data,
    std::uint32_t constantCount,
    BindingDomain domain)
{
	Require(data != nullptr, "Push-constant binding has no data.");
	Require(constantCount > 0, "Push-constant binding has no values.");

	if (domain == BindingDomain::Compute)
	{
		commandContext.SetComputePushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
		return;
	}
	if (domain == BindingDomain::RayTracing)
	{
		commandContext.SetRayTracingPushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
		return;
	}

	commandContext.SetPushConstants(compiledBinding.BindingIndex, constantCount, data, 0);
}
