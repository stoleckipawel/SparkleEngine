#include "../PCH.h"
#include "PipelineRuntimeLibrary.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <cassert>
#include <format>

namespace
{
	std::string FormatPackageId(const ShaderPackageDefinition& package)
	{
		return package.PackageId != nullptr ? std::string(package.PackageId) : std::string("<null>");
	}

	std::string FormatBindingLayoutId(const PipelineRuntimePackageRequest& request)
	{
		if (request.Package.BindingLayoutId != nullptr)
		{
			return std::string(request.Package.BindingLayoutId);
		}

		return request.BindingLayout != nullptr ? request.BindingLayout->GetDebugName() : std::string("<null>");
	}
}

bool PipelineRuntimeLibrary::LoadShaderPackage(
    RenderHardwareInterface& rhi,
    CookedShaderPackageCache& shaderPackageCache,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage*& outLoadedPackage,
    std::string& outErrorMessage)
{
	outLoadedPackage = nullptr;
	if (request.BindingLayout == nullptr)
	{
		outErrorMessage = std::format("Render pass '{}' requested shader package '{}' with no binding layout", request.PassName, FormatPackageId(request.Package));
		return false;
	}

	const RhiCapabilities& capabilities = rhi.GetCapabilities();
	const CookedShaderBinaryFormat requiredBinaryFormat = capabilities.RequiredShaderBinaryFormat;
	if (!shaderPackageCache.LoadPackage(request.Package, *request.BindingLayout, requiredBinaryFormat, outErrorMessage, outLoadedPackage))
	{
		const CookedShaderPackageLoadReport& loadReport = shaderPackageCache.GetLastLoadReport();
		outErrorMessage = std::format(
		    "Runtime validation rejected cooked shader package '{}' for pass '{}' ({}) with backend='{}' requiredFormat='{}' "
		    "bindingLayout='{}' expectedStages='{}' loadTimeUs={} packagePath='{}' - {}",
		    FormatPackageId(request.Package),
		    request.PassName,
		    request.PackageDeclarationName,
		    RhiBackendApiToString(capabilities.BackendApi),
		    CookedShaderBinaryFormatToString(requiredBinaryFormat),
		    FormatBindingLayoutId(request),
		    FormatShaderStageMask(request.Package.ExpectedStages),
		    loadReport.ElapsedMicroseconds,
		    loadReport.PackagePath.string(),
		    outErrorMessage);
		return false;
	}

	assert(outLoadedPackage != nullptr);
	outErrorMessage.clear();
	return true;
}

bool PipelineRuntimeLibrary::ValidatePackageCapabilities(
    RenderHardwareInterface& rhi,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage,
    std::string& outErrorMessage)
{
	const RhiCapabilities& capabilities = rhi.GetCapabilities();
	const CookedShaderPackageFeatureFlags packageFeatures = shaderPackage.GetHeader().PackageFeatures | request.Package.RequiredFeatures;
	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesAccelerationStructure) &&
	    !capabilities.RayTracing.SupportsRayTracing)
	{
		outErrorMessage = std::format(
		    "Render pass '{}' package '{}' requires acceleration-structure bindings, but backend '{}' reports ray tracing unsupported",
		    request.PassName,
		    FormatPackageId(request.Package),
		    RhiBackendApiToString(capabilities.BackendApi));
		return false;
	}

	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
	    !capabilities.RayTracing.SupportsInlineRayQuery)
	{
		outErrorMessage = std::format(
		    "Render pass '{}' package '{}' requires inline ray query, but backend '{}' reports inline ray query unsupported",
		    request.PassName,
		    FormatPackageId(request.Package),
		    RhiBackendApiToString(capabilities.BackendApi));
		return false;
	}

	outErrorMessage.clear();
	return true;
}

std::unique_ptr<RenderBindingLayout> PipelineRuntimeLibrary::CreateBindingLayout(
    RenderHardwareInterface& rhi,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage)
{
	RenderBindingLayoutCompileDesc bindingDesc{};
	bindingDesc.ParameterLayout = request.BindingLayout;
	bindingDesc.ShaderPackage = &shaderPackage;
	bindingDesc.AllowInputAssemblerInputLayout = request.AllowInputAssemblerInputLayout;
	bindingDesc.DebugName = request.BindingLayoutDebugName;
	return rhi.GetPipelineService().CreateBindingLayout(bindingDesc);
}

std::unique_ptr<RenderPipelineState> PipelineRuntimeLibrary::CreateGraphicsPipelineState(
    RenderHardwareInterface& rhi,
    const GraphicsPipelineStateDesc& pipelineDesc)
{
	return rhi.GetPipelineService().CreateGraphicsPipelineState(pipelineDesc);
}

std::unique_ptr<RenderPipelineState> PipelineRuntimeLibrary::CreateComputePipelineState(
    RenderHardwareInterface& rhi,
    const ComputePipelineStateDesc& pipelineDesc)
{
	return rhi.GetPipelineService().CreateComputePipelineState(pipelineDesc);
}
