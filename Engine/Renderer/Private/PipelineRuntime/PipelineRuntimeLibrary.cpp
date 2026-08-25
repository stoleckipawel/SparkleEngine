#include "../PCH.h"

#include "PipelineRuntimeLibrary.h"

#include "Core/Public/Diagnostics/Error.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <format>

void PipelineRuntimeLibrary::ValidateShaderCapabilities(
    RenderHardwareInterface& renderHardwareInterface,
    std::string_view shaderName,
    const ResolvedShader& shader)
{
	if (!shader.IsValid())
	{
		throw Diagnostics::Error(std::format("Shader '{}' did not resolve through the active global shader map.", shaderName));
	}
	const RhiCapabilities& capabilities = renderHardwareInterface.GetCapabilities();
	if (HasShaderFeature(shader.Entry->Features, ShaderFeatureFlags::UsesAccelerationStructure)
	    && !capabilities.RayTracing.SupportsAccelerationStructure)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Shader '{}' requires ray tracing, but backend '{}' does not support it.",
		        shaderName,
		        RhiBackendApiToString(capabilities.BackendApi)));
	}
	if (HasShaderFeature(shader.Entry->Features, ShaderFeatureFlags::UsesInlineRayQuery) && !capabilities.RayTracing.SupportsInlineRayQuery)
	{
		throw Diagnostics::Error(
		    std::format(
		        "Shader '{}' requires inline ray query, but backend '{}' does not support it.",
		        shaderName,
		        RhiBackendApiToString(capabilities.BackendApi)));
	}
	if (HasShaderFeature(shader.Entry->Features, ShaderFeatureFlags::UsesDescriptorIndexing)
	    && (!capabilities.DescriptorIndexing.SupportsSampledImageArrayNonUniformIndexing
	        || !capabilities.DescriptorIndexing.SupportsPartiallyBoundDescriptorArrays))
	{
		throw Diagnostics::Error(
		    std::format(
		        "Shader '{}' requires descriptor indexing, but backend '{}' does not support it.",
		        shaderName,
		        RhiBackendApiToString(capabilities.BackendApi)));
	}
}

std::unique_ptr<RenderBindingLayout> PipelineRuntimeLibrary::CreateBindingLayout(
    RenderHardwareInterface& renderHardwareInterface,
    const PassParameterLayout& parameterLayout,
    std::span<const ResolvedShader> shaders,
    bool allowInputAssemblerInputLayout,
    const wchar_t* debugName)
{
	RenderBindingLayoutCompileDesc desc;
	desc.ParameterLayout = &parameterLayout;
	desc.Shaders = shaders;
	desc.AllowInputAssemblerInputLayout = allowInputAssemblerInputLayout;
	desc.DebugName = debugName;
	std::unique_ptr<RenderBindingLayout> layout = renderHardwareInterface.GetPipelineService().CreateBindingLayout(desc);
	if (!layout)
	{
		throw Diagnostics::Error("Shader binding-layout creation failed.");
	}
	return layout;
}

std::unique_ptr<RenderPipeline> PipelineRuntimeLibrary::CreateGraphicsPipeline(
    RenderHardwareInterface& renderHardwareInterface,
    const GraphicsPipelineDesc& pipelineDesc)
{
	std::unique_ptr<RenderPipeline> pipeline = renderHardwareInterface.GetPipelineService().CreateGraphicsPipeline(pipelineDesc);
	if (!pipeline)
	{
		throw Diagnostics::Error("Graphics pipeline creation failed.");
	}
	return pipeline;
}

std::unique_ptr<RenderPipeline> PipelineRuntimeLibrary::CreateComputePipeline(
    RenderHardwareInterface& renderHardwareInterface,
    const ComputePipelineDesc& pipelineDesc)
{
	std::unique_ptr<RenderPipeline> pipeline = renderHardwareInterface.GetPipelineService().CreateComputePipeline(pipelineDesc);
	if (!pipeline)
	{
		throw Diagnostics::Error("Compute pipeline creation failed.");
	}
	return pipeline;
}
