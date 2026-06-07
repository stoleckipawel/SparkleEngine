#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cassert>
#include <string_view>

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

void PassBinder::BindGraphics(
    RenderCommandContext& cmd,
	const FrameGraphResourceCommands& resources,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool bindLayout)
{
	BindImpl(cmd, resources, renderHardwareInterface, layout, parameterSet, bindingNames, overrides, bindLayout, false);
}

void PassBinder::BindCompute(
    RenderCommandContext& cmd,
	const FrameGraphResourceCommands& resources,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool bindLayout)
{
	BindImpl(cmd, resources, renderHardwareInterface, layout, parameterSet, bindingNames, overrides, bindLayout, true);
}

void PassBinder::BindImpl(
    RenderCommandContext& cmd,
	const FrameGraphResourceCommands& resources,
    RenderHardwareInterface* renderHardwareInterface,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool bindLayout,
    bool isCompute)
{
	assert(parameterSet.HasLayout());
	if (bindingNames.empty())
	{
		const PassParameterLayout* parameterLayout = parameterSet.GetLayout();
		const PassParameterLayout& compiledLayout = layout.GetParameterLayout();
		assert(parameterLayout != nullptr);
		const bool sameLayoutInstance = parameterLayout == &compiledLayout;
		const bool matchingParameterShape = parameterLayout->GetParameterCount() == compiledLayout.GetParameterCount();
		assert(sameLayoutInstance || matchingParameterShape);
	}

	if (bindLayout && isCompute)
	{
		cmd.SetComputeBindingLayout(layout);
	}
	else if (bindLayout)
	{
		cmd.SetGraphicsBindingLayout(layout);
	}

	if (bindingNames.empty())
	{
		for (std::size_t bindingIndex = 0; bindingIndex < layout.GetBindingCount(); ++bindingIndex)
		{
			const CompiledBinding& compiledBinding = layout.GetBindings()[bindingIndex];
			BindCompiledBinding(
			    cmd,
			    resources,
			    renderHardwareInterface,
			    compiledBinding,
			    parameterSet.FindBinding(compiledBinding.Name),
			    overrides,
			    isCompute);
		}
		return;
	}

	for (const char* bindingName : bindingNames)
	{
		bool boundAny = false;
		for (std::size_t bindingIndex = 0; bindingIndex < layout.GetBindingCount(); ++bindingIndex)
		{
			const CompiledBinding& compiledBinding = layout.GetBindings()[bindingIndex];
			if (compiledBinding.Name == nullptr || std::string_view(compiledBinding.Name) != bindingName)
			{
				continue;
			}

			BindCompiledBinding(
			    cmd,
			    resources,
			    renderHardwareInterface,
			    compiledBinding,
			    parameterSet.FindBinding(bindingName),
			    overrides,
			    isCompute);
			boundAny = true;
		}

		assert(boundAny);
	}
}

void PassBinder::BindCompiledBinding(
    RenderCommandContext& cmd,
	const FrameGraphResourceCommands& resources,
    RenderHardwareInterface* renderHardwareInterface,
    const CompiledBinding& compiledBinding,
    const PassParameterBinding* parameterBinding,
    const PassBindingOverrides* overrides,
    bool isCompute)
{
	switch (compiledBinding.Type)
	{
		case CompiledBindingType::ConstantBuffer:
		{
			const PassBindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::ConstantBufferView) : nullptr;
			if (bindingOverride != nullptr)
			{
				BindGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
				return;
			}

			assert(renderHardwareInterface != nullptr);
			assert(parameterBinding != nullptr);
			const PassParameterUniformBindingData* uniformData = parameterBinding->AsUniformData();
			assert(uniformData != nullptr);
			BindGpuAddress(
			    cmd,
			    compiledBinding,
			    renderHardwareInterface->AllocateUniformConstantBuffer(
			        uniformData->Data,
			        uniformData->SizeInBytes),
			    isCompute);
			return;
		}
		case CompiledBindingType::ReadOnlyAddress:
		{
			const PassBindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::ShaderResourceView) : nullptr;
			if (bindingOverride != nullptr)
			{
				BindGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
				return;
			}

			assert(parameterBinding != nullptr);
			const PassParameterAccelerationStructureBindingData* accelerationStructureData =
			    parameterBinding->AsAccelerationStructureData();
			assert(accelerationStructureData != nullptr);
			const RhiGpuVirtualAddress gpuAddress = accelerationStructureData->Handle.IsValid()
			                                            ? resources.ResolveAccelerationStructureGpuAddress(
			                                                  accelerationStructureData->Handle)
			                                            : accelerationStructureData->GpuAddress;
			BindGpuAddress(cmd, compiledBinding, gpuAddress, isCompute);
			return;
		}
		case CompiledBindingType::ReadWriteAddress:
		{
			assert(overrides != nullptr);
			const PassBindingOverride* bindingOverride =
			    overrides->Find(compiledBinding.Name, PassBindingOverrideType::UnorderedAccessView);
			assert(bindingOverride != nullptr);
			BindGpuAddress(cmd, compiledBinding, bindingOverride->GpuAddress, isCompute);
			return;
		}
		case CompiledBindingType::ReadOnlyResourceTable:
		{
			if (TryBindDescriptorTableOverride(cmd, compiledBinding, overrides, isCompute))
			{
				return;
			}

			assert(parameterBinding != nullptr);
			if (const PassParameterDescriptorTableBindingData* descriptorTableData = parameterBinding->AsDescriptorTableData())
			{
				BindDescriptorTable(cmd, compiledBinding, descriptorTableData->Table, isCompute);
				return;
			}

			if (const PassParameterTextureBindingData* textureData = parameterBinding->AsTextureData())
			{
				assert(textureData->Handles.size() == 1);
				BindDescriptorTable(cmd, compiledBinding, resources.ResolveShaderResourceView(textureData->Handles[0]), isCompute);
				return;
			}

			const PassParameterBufferBindingData* bufferData = parameterBinding->AsBufferData();
			assert(bufferData != nullptr);
			assert(bufferData->Handles.size() == 1);
			BindDescriptorTable(cmd, compiledBinding, resources.ResolveShaderResourceView(bufferData->Handles[0]), isCompute);
			return;
		}
		case CompiledBindingType::ReadWriteResourceTable:
		{
			if (TryBindDescriptorTableOverride(cmd, compiledBinding, overrides, isCompute))
			{
				return;
			}

			assert(parameterBinding != nullptr);
			if (const PassParameterDescriptorTableBindingData* descriptorTableData = parameterBinding->AsDescriptorTableData())
			{
				BindDescriptorTable(cmd, compiledBinding, descriptorTableData->Table, isCompute);
				return;
			}

			if (const PassParameterTextureBindingData* textureData = parameterBinding->AsTextureData())
			{
				assert(textureData->Handles.size() == 1);
				BindDescriptorTable(cmd, compiledBinding, resources.ResolveUnorderedAccessView(textureData->Handles[0]), isCompute);
				return;
			}

			const PassParameterBufferBindingData* bufferData = parameterBinding->AsBufferData();
			assert(bufferData != nullptr);
			assert(bufferData->Handles.size() == 1);
			BindDescriptorTable(cmd, compiledBinding, resources.ResolveUnorderedAccessView(bufferData->Handles[0]), isCompute);
			return;
		}
		case CompiledBindingType::SamplerTable:
		{
			if (TryBindDescriptorTableOverride(cmd, compiledBinding, overrides, isCompute))
			{
				return;
			}

			assert(renderHardwareInterface != nullptr);
			assert(parameterBinding != nullptr);
			const PassParameterSamplerBindingData* samplerData = parameterBinding->AsSamplerData();
			assert(samplerData != nullptr);
			const RhiDescriptorTableBinding samplerBinding = renderHardwareInterface->GetSharedSamplerBinding(samplerData->Desc);
			assert(static_cast<bool>(samplerBinding));
			BindDescriptorTable(cmd, compiledBinding, samplerBinding, isCompute);
			return;
		}
		case CompiledBindingType::PushConstants:
		{
			const PassBindingOverride* bindingOverride =
			    overrides != nullptr ? overrides->Find(compiledBinding.Name, PassBindingOverrideType::PushConstants) : nullptr;
			if (bindingOverride != nullptr)
			{
				BindPushConstants(cmd, compiledBinding, bindingOverride->ConstantsData, bindingOverride->ConstantCount, isCompute);
				return;
			}

			assert(parameterBinding != nullptr);
			const PassParameterUniformBindingData* uniformData = parameterBinding->AsUniformData();
			assert(uniformData != nullptr);
			BindPushConstants(
			    cmd,
			    compiledBinding,
			    uniformData->Data,
			    uniformData->SizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
			    isCompute);
			return;
		}
		default:
			assert(false);
			return;
	}
}

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
