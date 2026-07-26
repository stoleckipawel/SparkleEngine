#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cassert>
#include <string_view>

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

			const RhiGpuVirtualAddress gpuAddress =
			    renderHardwareInterface->GetUploadService().AllocateUniformConstantBuffer(
			        cmd.GetRenderCommandList(),
			        uniformData->Data,
			        uniformData->SizeInBytes);
			BindGpuAddress(cmd, compiledBinding, gpuAddress, isCompute);
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
				if (descriptorTableData->Table)
				{
					BindDescriptorTable(cmd, compiledBinding, descriptorTableData->Table, isCompute);
				}
				else
				{
					BindDescriptorTable(cmd, compiledBinding, descriptorTableData->GpuHandle, isCompute);
				}
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
				if (descriptorTableData->Table)
				{
					BindDescriptorTable(cmd, compiledBinding, descriptorTableData->Table, isCompute);
				}
				else
				{
					BindDescriptorTable(cmd, compiledBinding, descriptorTableData->GpuHandle, isCompute);
				}
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
			const RhiDescriptorTableBinding samplerBinding =
			    renderHardwareInterface->GetDescriptorService().GetSharedSamplerBinding(samplerData->Desc);
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
