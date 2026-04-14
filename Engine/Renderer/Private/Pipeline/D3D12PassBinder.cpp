#include "../PCH.h"
#include "Pipeline/D3D12PassBinder.h"

#include "RHI/Public/D3D12/Pipeline/D3D12BindingLayout.h"

#include "GPU/CommandContext.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <cassert>

void D3D12PassBindingOverrides::SetConstantBufferView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	m_overrides.push_back(
	    D3D12BindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = D3D12BindingOverrideType::ConstantBufferView,
	        .GpuAddress = gpuAddress});
}

void D3D12PassBindingOverrides::SetShaderResourceView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	m_overrides.push_back(
	    D3D12BindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = D3D12BindingOverrideType::ShaderResourceView,
	        .GpuAddress = gpuAddress});
}

void D3D12PassBindingOverrides::SetUnorderedAccessView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	m_overrides.push_back(
	    D3D12BindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = D3D12BindingOverrideType::UnorderedAccessView,
	        .GpuAddress = gpuAddress});
}

void D3D12PassBindingOverrides::SetDescriptorTable(const char* name, D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable)
{
	m_overrides.push_back(
	    D3D12BindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = D3D12BindingOverrideType::DescriptorTable,
	        .DescriptorTable = descriptorTable});
}

void D3D12PassBindingOverrides::SetRootConstants(const char* name, const void* data, std::uint32_t constantCount)
{
	m_overrides.push_back(
	    D3D12BindingOverride{
	        .Name = name != nullptr ? name : "",
	        .Type = D3D12BindingOverrideType::RootConstants,
	        .ConstantsData = data,
	        .ConstantCount = constantCount});
}

const D3D12BindingOverride* D3D12PassBindingOverrides::Find(const char* name, D3D12BindingOverrideType type) const noexcept
{
	if (name == nullptr)
	{
		return nullptr;
	}

	for (const D3D12BindingOverride& bindingOverride : m_overrides)
	{
		if (bindingOverride.Type == type && bindingOverride.Name == name)
		{
			return &bindingOverride;
		}
	}

	return nullptr;
}

void D3D12PassBinder::BindGraphics(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const D3D12BindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const D3D12PassBindingOverrides* overrides)
{
	BindImpl(cmd, frameGraph, layout, parameterSet, bindingNames, overrides, false);
}

void D3D12PassBinder::BindCompute(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const D3D12BindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const D3D12PassBindingOverrides* overrides)
{
	BindImpl(cmd, frameGraph, layout, parameterSet, bindingNames, overrides, true);
}

void D3D12PassBinder::BindImpl(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const D3D12BindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const D3D12PassBindingOverrides* overrides,
    bool isCompute)
{
	assert(parameterSet.HasLayout());
	if (bindingNames.empty())
	{
		assert(parameterSet.GetLayout() == &layout.GetParameterLayout());
	}

	if (isCompute)
	{
		cmd.SetComputeRootSignature(layout.GetRootSignature().GetRaw());
	}
	else
	{
		cmd.SetRootSignature(layout.GetRootSignature().GetRaw());
	}

	if (bindingNames.empty())
	{
		for (const D3D12CompiledBinding& compiledBinding : layout.GetBindings())
		{
			BindCompiledBinding(
			    cmd,
			    frameGraph,
			    compiledBinding,
			    parameterSet.FindBinding(compiledBinding.Name.c_str()),
			    overrides,
			    isCompute);
		}
		return;
	}

	for (const char* bindingName : bindingNames)
	{
		const D3D12CompiledBinding* compiledBinding = layout.FindBinding(bindingName);
		assert(compiledBinding != nullptr);
		BindCompiledBinding(cmd, frameGraph, *compiledBinding, parameterSet.FindBinding(bindingName), overrides, isCompute);
	}
}

void D3D12PassBinder::BindCompiledBinding(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
    const D3D12CompiledBinding& compiledBinding,
    const PassParameterBinding* parameterBinding,
    const D3D12PassBindingOverrides* overrides,
    bool isCompute)
{
	switch (compiledBinding.Type)
	{
		case D3D12CompiledBindingType::RootConstantBufferView:
		{
			assert(overrides != nullptr);
			const D3D12BindingOverride* bindingOverride =
			    overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::ConstantBufferView);
			assert(bindingOverride != nullptr);
			BindRootGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case D3D12CompiledBindingType::RootShaderResourceView:
		{
			assert(overrides != nullptr);
			const D3D12BindingOverride* bindingOverride =
			    overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::ShaderResourceView);
			assert(bindingOverride != nullptr);
			BindRootGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case D3D12CompiledBindingType::RootUnorderedAccessView:
		{
			assert(overrides != nullptr);
			const D3D12BindingOverride* bindingOverride =
			    overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::UnorderedAccessView);
			assert(bindingOverride != nullptr);
			BindRootGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case D3D12CompiledBindingType::DescriptorTableShaderResourceView:
		{
			const D3D12BindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::DescriptorTable) : nullptr;
			if (bindingOverride != nullptr)
			{
				BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
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
		case D3D12CompiledBindingType::DescriptorTableUnorderedAccessView:
		{
			const D3D12BindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::DescriptorTable) : nullptr;
			if (bindingOverride != nullptr)
			{
				BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
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
		case D3D12CompiledBindingType::DescriptorTableSampler:
		{
			assert(overrides != nullptr);
			const D3D12BindingOverride* bindingOverride =
			    overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::DescriptorTable);
			assert(bindingOverride != nullptr);
			BindDescriptorTable(cmd, compiledBinding, bindingOverride->DescriptorTable, isCompute);
			return;
		}
		case D3D12CompiledBindingType::RootConstants:
		{
			const D3D12BindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name.c_str(), D3D12BindingOverrideType::RootConstants) : nullptr;
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

void D3D12PassBinder::BindRootGpuAddress(
    CommandContext& cmd,
    const D3D12CompiledBinding& compiledBinding,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
    bool isCompute)
{
	if (isCompute)
	{
		switch (compiledBinding.Type)
		{
			case D3D12CompiledBindingType::RootConstantBufferView:
				cmd.BindComputeRootConstantBuffer(compiledBinding.RootParameterIndex, gpuAddress);
				return;
			case D3D12CompiledBindingType::RootShaderResourceView:
				cmd.BindComputeRootShaderResourceView(compiledBinding.RootParameterIndex, gpuAddress);
				return;
			case D3D12CompiledBindingType::RootUnorderedAccessView:
				cmd.BindComputeRootUnorderedAccessView(compiledBinding.RootParameterIndex, gpuAddress);
				return;
			default:
				assert(false);
				return;
		}
	}

	switch (compiledBinding.Type)
	{
		case D3D12CompiledBindingType::RootConstantBufferView:
			cmd.BindConstantBuffer(compiledBinding.RootParameterIndex, gpuAddress);
			return;
		case D3D12CompiledBindingType::RootShaderResourceView:
			cmd.BindRootShaderResourceView(compiledBinding.RootParameterIndex, gpuAddress);
			return;
		case D3D12CompiledBindingType::RootUnorderedAccessView:
			cmd.BindRootUnorderedAccessView(compiledBinding.RootParameterIndex, gpuAddress);
			return;
		default:
			assert(false);
			return;
	}
}

void D3D12PassBinder::BindDescriptorTable(
    CommandContext& cmd,
    const D3D12CompiledBinding& compiledBinding,
    D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable,
    bool isCompute)
{
	if (isCompute)
	{
		cmd.BindComputeDescriptorTable(compiledBinding.RootParameterIndex, descriptorTable);
		return;
	}

	cmd.BindDescriptorTable(compiledBinding.RootParameterIndex, descriptorTable);
}

void D3D12PassBinder::BindRootConstants(
    CommandContext& cmd,
    const D3D12CompiledBinding& compiledBinding,
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