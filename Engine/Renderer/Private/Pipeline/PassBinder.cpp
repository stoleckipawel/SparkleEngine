#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "GPU/CommandContext.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <cassert>

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

void PassBindingOverrides::SetDescriptorTable(const char* name, RhiDescriptorTableHandle descriptorTable)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::DescriptorTable,
	        .LogicalDescriptorTable = descriptorTable,
	        .DescriptorTableKind = DescriptorTableOverrideKind::LogicalTable});
}

void PassBindingOverrides::SetRootConstants(const char* name, const void* data, std::uint32_t constantCount)
{
	m_overrides.push_back(
	    PassBindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = PassBindingOverrideType::RootConstants,
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

void PassBinder::BindGraphics(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides)
{
	BindImpl(cmd, frameGraph, layout, parameterSet, bindingNames, overrides, false);
}

void PassBinder::BindCompute(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides)
{
	BindImpl(cmd, frameGraph, layout, parameterSet, bindingNames, overrides, true);
}

void PassBinder::BindImpl(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool isCompute)
{
	assert(parameterSet.HasLayout());
	if (bindingNames.empty())
	{
		assert(parameterSet.GetLayout() == &layout.GetParameterLayout());
	}

	if (isCompute)
	{
		cmd.SetComputeBindingLayout(layout);
	}
	else
	{
		cmd.SetGraphicsBindingLayout(layout);
	}

	if (bindingNames.empty())
	{
		for (std::size_t bindingIndex = 0; bindingIndex < layout.GetBindingCount(); ++bindingIndex)
		{
			const CompiledBinding& compiledBinding = layout.GetBindings()[bindingIndex];
			BindCompiledBinding(cmd, frameGraph, compiledBinding, parameterSet.FindBinding(compiledBinding.Name), overrides, isCompute);
		}
		return;
	}

	for (const char* bindingName : bindingNames)
	{
		const CompiledBinding* compiledBinding = layout.FindBinding(bindingName);
		assert(compiledBinding != nullptr);
		BindCompiledBinding(cmd, frameGraph, *compiledBinding, parameterSet.FindBinding(bindingName), overrides, isCompute);
	}
}

void PassBinder::BindCompiledBinding(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const CompiledBinding& compiledBinding,
    const PassParameterBinding* parameterBinding,
    const PassBindingOverrides* overrides,
    bool isCompute)
{
	switch (compiledBinding.Type)
	{
		case CompiledBindingType::RootConstantBufferView:
		{
			assert(overrides != nullptr);
			const PassBindingOverride* bindingOverride = overrides->Find(compiledBinding.Name, PassBindingOverrideType::ConstantBufferView);
			assert(bindingOverride != nullptr);
			BindRootGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case CompiledBindingType::RootShaderResourceView:
		{
			assert(overrides != nullptr);
			const PassBindingOverride* bindingOverride = overrides->Find(compiledBinding.Name, PassBindingOverrideType::ShaderResourceView);
			assert(bindingOverride != nullptr);
			BindRootGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case CompiledBindingType::RootUnorderedAccessView:
		{
			assert(overrides != nullptr);
			const PassBindingOverride* bindingOverride =
			    overrides->Find(compiledBinding.Name, PassBindingOverrideType::UnorderedAccessView);
			assert(bindingOverride != nullptr);
			BindRootGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case CompiledBindingType::DescriptorTableShaderResourceView:
		{
			const PassBindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::DescriptorTable) : nullptr;
			if (bindingOverride != nullptr)
			{
				if (bindingOverride->DescriptorTableKind == DescriptorTableOverrideKind::LogicalTable)
				{
					BindDescriptorTable(cmd, compiledBinding, bindingOverride->LogicalDescriptorTable, isCompute);
				}
				else
				{
					BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
				}
				return;
			}

			assert(parameterBinding != nullptr);
			if (parameterBinding->Kind == PassParameterValueKind::Texture)
			{
				assert(parameterBinding->Textures.size() == 1);
				BindDescriptorTable(cmd, compiledBinding, frameGraph.ResolveShaderResourceView(parameterBinding->Textures[0]), isCompute);
				return;
			}

			assert(parameterBinding->Kind == PassParameterValueKind::Buffer);
			assert(parameterBinding->Buffers.size() == 1);
			BindDescriptorTable(cmd, compiledBinding, frameGraph.ResolveShaderResourceView(parameterBinding->Buffers[0]), isCompute);
			return;
		}
		case CompiledBindingType::DescriptorTableUnorderedAccessView:
		{
			const PassBindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::DescriptorTable) : nullptr;
			if (bindingOverride != nullptr)
			{
				if (bindingOverride->DescriptorTableKind == DescriptorTableOverrideKind::LogicalTable)
				{
					BindDescriptorTable(cmd, compiledBinding, bindingOverride->LogicalDescriptorTable, isCompute);
				}
				else
				{
					BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
				}
				return;
			}

			assert(parameterBinding != nullptr);
			if (parameterBinding->Kind == PassParameterValueKind::Texture)
			{
				assert(parameterBinding->Textures.size() == 1);
				BindDescriptorTable(cmd, compiledBinding, frameGraph.ResolveUnorderedAccessView(parameterBinding->Textures[0]), isCompute);
				return;
			}

			assert(parameterBinding->Kind == PassParameterValueKind::Buffer);
			assert(parameterBinding->Buffers.size() == 1);
			BindDescriptorTable(cmd, compiledBinding, frameGraph.ResolveUnorderedAccessView(parameterBinding->Buffers[0]), isCompute);
			return;
		}
		case CompiledBindingType::DescriptorTableSampler:
		{
			assert(overrides != nullptr);
			const PassBindingOverride* bindingOverride = overrides->Find(compiledBinding.Name, PassBindingOverrideType::DescriptorTable);
			assert(bindingOverride != nullptr);
			if (bindingOverride->DescriptorTableKind == DescriptorTableOverrideKind::LogicalTable)
			{
				BindDescriptorTable(cmd, compiledBinding, bindingOverride->LogicalDescriptorTable, isCompute);
			}
			else
			{
				BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
			}
			return;
		}
		case CompiledBindingType::RootConstants:
		{
			const PassBindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::RootConstants) : nullptr;
			if (bindingOverride != nullptr)
			{
				BindRootConstants(cmd, compiledBinding, bindingOverride->ConstantsData, bindingOverride->ConstantCount, isCompute);
				return;
			}

			assert(parameterBinding != nullptr);
			assert(parameterBinding->Kind == PassParameterValueKind::UniformData);
			BindRootConstants(
			    cmd,
			    compiledBinding,
			    parameterBinding->UniformData,
			    parameterBinding->UniformDataSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
			    isCompute);
			return;
		}
		default:
			assert(false);
			return;
	}
}

void PassBinder::BindRootGpuAddress(
    CommandContext& cmd,
    const CompiledBinding& compiledBinding,
    RhiGpuVirtualAddress gpuAddress,
    bool isCompute)
{
	if (isCompute)
	{
		switch (compiledBinding.Type)
		{
			case CompiledBindingType::RootConstantBufferView:
				cmd.BindComputeRootConstantBuffer(compiledBinding.RootParameterIndex, gpuAddress);
				return;
			case CompiledBindingType::RootShaderResourceView:
				cmd.BindComputeRootShaderResourceView(compiledBinding.RootParameterIndex, gpuAddress);
				return;
			case CompiledBindingType::RootUnorderedAccessView:
				cmd.BindComputeRootUnorderedAccessView(compiledBinding.RootParameterIndex, gpuAddress);
				return;
			default:
				assert(false);
				return;
		}
	}

	switch (compiledBinding.Type)
	{
		case CompiledBindingType::RootConstantBufferView:
			cmd.BindConstantBuffer(compiledBinding.RootParameterIndex, gpuAddress);
			return;
		case CompiledBindingType::RootShaderResourceView:
			cmd.BindRootShaderResourceView(compiledBinding.RootParameterIndex, gpuAddress);
			return;
		case CompiledBindingType::RootUnorderedAccessView:
			cmd.BindRootUnorderedAccessView(compiledBinding.RootParameterIndex, gpuAddress);
			return;
		default:
			assert(false);
			return;
	}
}

void PassBinder::BindDescriptorTable(
    CommandContext& cmd,
    const CompiledBinding& compiledBinding,
    RhiGpuDescriptorHandle descriptorTable,
    bool isCompute)
{
	if (isCompute)
	{
		cmd.BindComputeDescriptorTable(compiledBinding.RootParameterIndex, descriptorTable);
		return;
	}

	cmd.BindDescriptorTable(compiledBinding.RootParameterIndex, descriptorTable);
}

void PassBinder::BindDescriptorTable(
    CommandContext& cmd,
    const CompiledBinding& compiledBinding,
    RhiDescriptorTableHandle descriptorTable,
    bool isCompute)
{
	if (isCompute)
	{
		cmd.BindComputeDescriptorTable(compiledBinding.RootParameterIndex, descriptorTable);
		return;
	}

	cmd.BindDescriptorTable(compiledBinding.RootParameterIndex, descriptorTable);
}

void PassBinder::BindRootConstants(
    CommandContext& cmd,
    const CompiledBinding& compiledBinding,
    const void* data,
    std::uint32_t constantCount,
    bool isCompute)
{
	assert(data != nullptr);
	assert(constantCount > 0);

	if (isCompute)
	{
		cmd.SetComputeRoot32BitConstants(compiledBinding.RootParameterIndex, constantCount, data, 0);
		return;
	}

	cmd.SetRoot32BitConstants(compiledBinding.RootParameterIndex, constantCount, data, 0);
}