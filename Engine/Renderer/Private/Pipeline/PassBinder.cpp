#include "../PCH.h"
#include "Pipeline/PassBinder.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <string>
#include <string_view>

static const auto g_passBinderLogger = Logging::GetOrCreateLogger("Renderer.PassBinder");

struct PassBinder::BindingRequest final
{
	RenderCommandContext& CommandContext;
	const FrameGraphResourceCommands& Resources;
	RenderHardwareInterface& HardwareInterface;
	const CompiledBinding& Binding;
	const PassParameterBinding* Parameters;
	const PassBindingOverrides* Overrides;
	bool IsCompute = false;
};

void PassBinder::BindGraphics(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool bindLayout)
{
	BindImpl(commandContext, resources, layout, parameterSet, bindingNames, overrides, bindLayout, false);
}

void PassBinder::BindCompute(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool bindLayout)
{
	BindImpl(commandContext, resources, layout, parameterSet, bindingNames, overrides, bindLayout, true);
}

void PassBinder::BindImpl(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& layout,
    const PassParameterSet& parameterSet,
    std::span<const char* const> bindingNames,
    const PassBindingOverrides* overrides,
    bool bindLayout,
    bool isCompute)
{
	Require(parameterSet.HasLayout(), "Pass binding requires a parameter set with a compiled layout.");
	if (bindingNames.empty())
	{
		const PassParameterLayout* parameterLayout = parameterSet.GetLayout();
		const PassParameterLayout& compiledLayout = layout.GetParameterLayout();
		Require(parameterLayout != nullptr, "Pass parameter layout is unavailable.");
		Require(parameterLayout->Matches(compiledLayout), "Pass parameter layout does not match the compiled binding layout.");
	}

	if (bindLayout && isCompute)
	{
		commandContext.SetComputeBindingLayout(layout);
	}
	else if (bindLayout)
	{
		commandContext.SetGraphicsBindingLayout(layout);
	}

	if (bindingNames.empty())
	{
		for (std::size_t bindingIndex = 0; bindingIndex < layout.GetBindingCount(); ++bindingIndex)
		{
			const CompiledBinding& compiledBinding = layout.GetBindings()[bindingIndex];
			BindCompiledBinding(
			    commandContext,
			    resources,
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

			BindCompiledBinding(commandContext, resources, compiledBinding, parameterSet.FindBinding(bindingName), overrides, isCompute);
			boundAny = true;
		}

		Require(boundAny, "A requested pass binding name is absent from the compiled binding layout.");
	}
}

void PassBinder::BindCompiledBinding(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const CompiledBinding& compiledBinding,
    const PassParameterBinding* parameterBinding,
    const PassBindingOverrides* overrides,
    bool isCompute)
{
	const BindingRequest request{
	    .CommandContext = commandContext,
	    .Resources = resources,
	    .HardwareInterface = resources.GetRenderHardwareInterface(),
	    .Binding = compiledBinding,
	    .Parameters = parameterBinding,
	    .Overrides = overrides,
	    .IsCompute = isCompute};
	switch (compiledBinding.Type)
	{
		case CompiledBindingType::ConstantBuffer:
			BindConstantBuffer(request);
			return;
		case CompiledBindingType::ReadOnlyAddress:
			BindReadOnlyAddress(request);
			return;
		case CompiledBindingType::ReadWriteAddress:
			BindReadWriteAddress(request);
			return;
		case CompiledBindingType::AccelerationStructure:
			BindAccelerationStructure(request);
			return;
		case CompiledBindingType::ReadOnlyResourceTable:
			BindResourceTable(request, false);
			return;
		case CompiledBindingType::ReadWriteResourceTable:
			BindResourceTable(request, true);
			return;
		case CompiledBindingType::SamplerTable:
			BindSamplerTable(request);
			return;
		case CompiledBindingType::PushConstants:
			BindPushConstantData(request);
			return;
		default:
			Diagnostics::Fatal(g_passBinderLogger, __FILE__, __LINE__, "Compiled pass binding has an unsupported type.");
	}
}

void PassBinder::BindConstantBuffer(const BindingRequest& request)
{
	const PassBindingOverride* bindingOverride =
	    request.Overrides != nullptr ? request.Overrides->Find(request.Binding.Name, PassBindingOverrideType::ConstantBufferView) : nullptr;
	if (bindingOverride != nullptr)
	{
		BindGpuAddress(request.CommandContext, request.Binding, bindingOverride->GpuAddress, request.IsCompute);
		return;
	}

	Require(request.Parameters != nullptr, "Constant-buffer binding is absent from the pass parameter set.");
	const PassParameterUniformBindingData* uniformData = request.Parameters->AsUniformData();
	Require(uniformData != nullptr, "Constant-buffer binding has incompatible parameter data.");
	const RhiGpuVirtualAddress gpuAddress = request.HardwareInterface.GetUploadService().AllocateUniformConstantBuffer(
	    request.CommandContext.GetRenderCommandList(),
	    uniformData->Data,
	    uniformData->SizeInBytes);
	BindGpuAddress(request.CommandContext, request.Binding, gpuAddress, request.IsCompute);
}

void PassBinder::BindReadOnlyAddress(const BindingRequest& request)
{
	const PassBindingOverride* bindingOverride =
	    request.Overrides != nullptr ? request.Overrides->Find(request.Binding.Name, PassBindingOverrideType::ShaderResourceView) : nullptr;
	if (bindingOverride != nullptr)
	{
		BindGpuAddress(request.CommandContext, request.Binding, bindingOverride->GpuAddress, request.IsCompute);
		return;
	}

	Require(false, "Read-only address binding requires an explicit pass override.");
}

void PassBinder::BindReadWriteAddress(const BindingRequest& request)
{
	Require(request.Overrides != nullptr, "Read-write address binding requires an explicit pass override.");
	const PassBindingOverride* bindingOverride =
	    request.Overrides->Find(request.Binding.Name, PassBindingOverrideType::UnorderedAccessView);
	Require(bindingOverride != nullptr, "Read-write address binding has no unordered-access override.");
	BindGpuAddress(request.CommandContext, request.Binding, bindingOverride->GpuAddress, request.IsCompute);
}

void PassBinder::BindAccelerationStructure(const BindingRequest& request)
{
	Require(request.Parameters != nullptr, "Acceleration-structure binding is absent from the pass parameter set.");
	const FrameGraphAccelerationStructureHandle* accelerationStructure = request.Parameters->AsAccelerationStructureHandle();
	Require(accelerationStructure != nullptr, "Acceleration-structure binding has incompatible parameter data.");
	const RhiResourceHandle resource = request.Resources.ResolveAccelerationStructure(*accelerationStructure);
	Require(static_cast<bool>(resource), "Acceleration-structure binding resolved to no resource.");
	if (request.IsCompute)
	{
		request.CommandContext.BindComputeAccelerationStructure(request.Binding.BindingIndex, resource);
		return;
	}
	request.CommandContext.BindAccelerationStructure(request.Binding.BindingIndex, resource);
}

void PassBinder::BindResourceTable(const BindingRequest& request, bool readWrite)
{
	const PassBindingOverride* bindingOverride =
	    request.Overrides != nullptr ? request.Overrides->Find(request.Binding.Name, PassBindingOverrideType::DescriptorTable) : nullptr;
	if (bindingOverride != nullptr)
	{
		BindDescriptorTableOverride(request.CommandContext, request.Binding, *bindingOverride, request.IsCompute);
		return;
	}

	Require(
	    request.Parameters != nullptr,
	    readWrite ? "Read-write resource binding is absent from the pass parameter set."
	              : "Read-only resource binding is absent from the pass parameter set.");
	if (const PassParameterDescriptorTableBindingData* descriptorTableData = request.Parameters->AsDescriptorTableData())
	{
		if (descriptorTableData->Table)
		{
			BindDescriptorTable(request.CommandContext, request.Binding, descriptorTableData->Table, request.IsCompute);
		}
		else
		{
			BindDescriptorTable(request.CommandContext, request.Binding, descriptorTableData->GpuHandle, request.IsCompute);
		}
		return;
	}

	if (const PassParameterTextureBindingData* textureData = request.Parameters->AsTextureData())
	{
		Require(textureData->Handles.size() == 1, "Texture binding must contain exactly one resource.");
		const RhiGpuDescriptorHandle view = readWrite ? request.Resources.ResolveUnorderedAccessView(textureData->Handles[0])
		                                              : request.Resources.ResolveShaderResourceView(textureData->Handles[0]);
		BindDescriptorTable(request.CommandContext, request.Binding, view, request.IsCompute);
		return;
	}

	const PassParameterBufferBindingData* bufferData = request.Parameters->AsBufferData();
	Require(
	    bufferData != nullptr,
	    readWrite ? "Read-write resource binding has incompatible parameter data."
	              : "Read-only resource binding has incompatible parameter data.");
	Require(bufferData->Handles.size() == 1, "Buffer binding must contain exactly one resource.");
	const RhiGpuDescriptorHandle view = readWrite ? request.Resources.ResolveUnorderedAccessView(bufferData->Handles[0])
	                                              : request.Resources.ResolveShaderResourceView(bufferData->Handles[0]);
	BindDescriptorTable(request.CommandContext, request.Binding, view, request.IsCompute);
}

void PassBinder::BindSamplerTable(const BindingRequest& request)
{
	const PassBindingOverride* bindingOverride =
	    request.Overrides != nullptr ? request.Overrides->Find(request.Binding.Name, PassBindingOverrideType::DescriptorTable) : nullptr;
	if (bindingOverride != nullptr)
	{
		BindDescriptorTableOverride(request.CommandContext, request.Binding, *bindingOverride, request.IsCompute);
		return;
	}

	Require(request.Parameters != nullptr, "Sampler binding is absent from the pass parameter set.");
	const PassParameterSamplerBindingData* samplerData = request.Parameters->AsSamplerData();
	Require(samplerData != nullptr, "Sampler binding has incompatible parameter data.");
	const RhiDescriptorTableBinding samplerBinding =
	    request.HardwareInterface.GetDescriptorService().GetSharedSamplerBinding(samplerData->Desc);
	Require(static_cast<bool>(samplerBinding), "Sampler binding did not resolve a descriptor table.");
	BindDescriptorTable(request.CommandContext, request.Binding, samplerBinding, request.IsCompute);
}

void PassBinder::BindPushConstantData(const BindingRequest& request)
{
	const PassBindingOverride* bindingOverride =
	    request.Overrides != nullptr ? request.Overrides->Find(request.Binding.Name, PassBindingOverrideType::PushConstants) : nullptr;
	if (bindingOverride != nullptr)
	{
		BindPushConstants(
		    request.CommandContext,
		    request.Binding,
		    bindingOverride->ConstantsData,
		    bindingOverride->ConstantCount,
		    request.IsCompute);
		return;
	}

	Require(request.Parameters != nullptr, "Push-constant binding is absent from the pass parameter set.");
	const PassParameterUniformBindingData* uniformData = request.Parameters->AsUniformData();
	Require(uniformData != nullptr, "Push-constant binding has incompatible parameter data.");
	BindPushConstants(
	    request.CommandContext,
	    request.Binding,
	    uniformData->Data,
	    uniformData->SizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
	    request.IsCompute);
}

void PassBinder::Require(bool condition, std::string_view message)
{
	if (!condition)
	{
		Diagnostics::Fatal(g_passBinderLogger, __FILE__, __LINE__, std::string(message));
	}
}
