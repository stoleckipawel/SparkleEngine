#pragma once

#include "RHI/Public/Pipeline/RhiPipelineDesc.h"
#include "RHI/Public/Shaders/CookedShaderPackageCache.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"

#include <memory>
#include <string_view>

class RenderHardwareInterface;

struct PipelineRuntimePackageRequest final
{
	std::string_view PassName;
	ShaderPackageDefinition Package;
	const PassParameterLayout* BindingLayout = nullptr;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* BindingLayoutDebugName = L"RenderPass_BindingLayout";
};

class PipelineRuntimeLibrary final
{
  public:
	PipelineRuntimeLibrary() = delete;

	static const LoadedShaderPackage& LoadShaderPackage(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request);

	static void ValidatePackageCapabilities(
	    RenderHardwareInterface& renderHardwareInterface,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage);

	static std::unique_ptr<RenderBindingLayout> CreateBindingLayout(
	    RenderHardwareInterface& renderHardwareInterface,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage);

	static std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(
	    RenderHardwareInterface& renderHardwareInterface,
	    const GraphicsPipelineDesc& pipelineDesc);

	static std::unique_ptr<RenderPipeline> CreateComputePipeline(
	    RenderHardwareInterface& renderHardwareInterface,
	    const ComputePipelineDesc& pipelineDesc);
};
