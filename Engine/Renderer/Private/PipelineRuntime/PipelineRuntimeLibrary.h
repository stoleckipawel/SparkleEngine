#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <memory>
#include <span>
#include <string_view>

class PassParameterLayout;
class RenderHardwareInterface;

class PipelineRuntimeLibrary final
{
public:
	PipelineRuntimeLibrary() = delete;

	static void ValidateShaderCapabilities(
	    RenderHardwareInterface& renderHardwareInterface,
	    std::string_view shaderName,
	    const ResolvedShader& shader);

	static std::unique_ptr<RenderBindingLayout> CreateBindingLayout(
	    RenderHardwareInterface& renderHardwareInterface,
	    const PassParameterLayout& parameterLayout,
	    std::span<const ResolvedShader> shaders,
	    bool allowInputAssemblerInputLayout,
	    const wchar_t* debugName);

	static std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(
	    RenderHardwareInterface& renderHardwareInterface,
	    const GraphicsPipelineDesc& pipelineDesc);

	static std::unique_ptr<RenderPipeline> CreateComputePipeline(
	    RenderHardwareInterface& renderHardwareInterface,
	    const ComputePipelineDesc& pipelineDesc);
};
