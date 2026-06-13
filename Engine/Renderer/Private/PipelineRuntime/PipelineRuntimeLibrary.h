#pragma once

#include "PipelineRuntime/PipelineRuntimeKey.h"
#include "RHI/Public/Pipeline/RhiPipelineStateDesc.h"
#include "RHI/Public/Shaders/CookedShaderPackageCache.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <memory>
#include <string>
#include <string_view>

class RenderHardwareInterface;

struct PipelineRuntimePackageRequest final
{
	std::string_view PassName;
	std::string_view PackageDeclarationName;
	ShaderPackageDefinition Package;
	const PassParameterLayout* BindingLayout = nullptr;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* BindingLayoutDebugName = L"RenderPass_BindingLayout";
};

class PipelineRuntimeLibrary final
{
  public:
	PipelineRuntimeLibrary() = delete;

	static bool LoadShaderPackage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage*& outLoadedPackage,
	    std::string& outErrorMessage);

	static bool ValidatePackageCapabilities(
	    RenderHardwareInterface& rhi,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage,
	    std::string& outErrorMessage);

	static std::unique_ptr<RenderBindingLayout> CreateBindingLayout(
	    RenderHardwareInterface& rhi,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage);

	static std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage,
	    const GraphicsPipelineStateDesc& pipelineDesc);

	static std::unique_ptr<RenderPipelineState> CreateComputePipelineState(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage,
	    const ComputePipelineStateDesc& pipelineDesc);

	static PipelineRuntimeKey BuildGraphicsPipelineKey(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage,
	    const GraphicsPipelineStateDesc& pipelineDesc);

	static PipelineRuntimeKey BuildComputePipelineKey(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage);
};
