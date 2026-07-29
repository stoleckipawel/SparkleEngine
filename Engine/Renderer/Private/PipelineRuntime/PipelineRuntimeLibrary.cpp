#include "../PCH.h"
#include "PipelineRuntimeLibrary.h"

#include "Core/Public/Diagnostics/Error.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <cassert>
#include <format>

class ShaderPackageIdentification final
{
  public:
	static std::string FormatPackageId(const ShaderPackageDefinition& package)
	{
		return package.PackageId != nullptr ? std::string(package.PackageId) : std::string("<null>");
	}
};

const LoadedShaderPackage& PipelineRuntimeLibrary::LoadShaderPackage(
    RenderHardwareInterface& renderHardwareInterface,
    CookedShaderPackageCache& shaderPackageCache,
    const PipelineRuntimePackageRequest& request)
{
	if (request.BindingLayout == nullptr)
	{
		throw Diagnostics::Error(std::format(
		    "Render pass '{}' requested shader package '{}' with no binding layout",
		    request.PassName,
		    ShaderPackageIdentification::FormatPackageId(request.Package)));
	}

	const RhiCapabilities& capabilities = renderHardwareInterface.GetCapabilities();
	const CookedShaderBinaryFormat backendBinaryFormat = capabilities.RuntimeShaderBinaryFormat;
	try
	{
		return shaderPackageCache.LoadPackage(request.Package, *request.BindingLayout, backendBinaryFormat);
	}
	catch (const Diagnostics::Error& error)
	{
		const CookedShaderPackageLoadReport& loadReport = shaderPackageCache.GetLastLoadReport();
		throw Diagnostics::Error(std::format(
		    "Runtime validation rejected cooked shader package '{}' for pass '{}' with backend='{}' backendFormat='{}' "
		    "bindingLayout='{}' expectedStages='{}' loadTimeUs={} packagePath='{}' - {}",
		    ShaderPackageIdentification::FormatPackageId(request.Package),
		    request.PassName,
		    RhiBackendApiToString(capabilities.BackendApi),
		    CookedShaderBinaryFormatToString(backendBinaryFormat),
		    request.BindingLayout->GetDebugName(),
		    FormatShaderStageMask(request.Package.ExpectedStages),
		    loadReport.ElapsedMicroseconds,
		    loadReport.PackagePath.string(),
		    error.what()));
	}
}

void PipelineRuntimeLibrary::ValidatePackageCapabilities(
    RenderHardwareInterface& renderHardwareInterface,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage)
{
	const RhiCapabilities& capabilities = renderHardwareInterface.GetCapabilities();
	const CookedShaderPackageFeatureFlags packageFeatures = shaderPackage.GetHeader().PackageFeatures | request.Package.RequiredFeatures;
	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesAccelerationStructure) &&
	    !capabilities.RayTracing.SupportsRayTracing)
	{
		throw Diagnostics::Error(std::format(
		    "Render pass '{}' package '{}' uses acceleration-structure bindings, but backend '{}' reports ray tracing unsupported",
		    request.PassName,
		    ShaderPackageIdentification::FormatPackageId(request.Package),
		    RhiBackendApiToString(capabilities.BackendApi)));
	}

	if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesInlineRayQuery) &&
	    !capabilities.RayTracing.SupportsInlineRayQuery)
	{
		throw Diagnostics::Error(std::format(
		    "Render pass '{}' package '{}' uses inline ray query, but backend '{}' reports inline ray query unsupported",
		    request.PassName,
		    ShaderPackageIdentification::FormatPackageId(request.Package),
		    RhiBackendApiToString(capabilities.BackendApi)));
	}
}

std::unique_ptr<RenderBindingLayout> PipelineRuntimeLibrary::CreateBindingLayout(
    RenderHardwareInterface& renderHardwareInterface,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage)
{
	RenderBindingLayoutCompileDesc bindingDesc{};
	bindingDesc.ParameterLayout = request.BindingLayout;
	bindingDesc.ShaderPackage = &shaderPackage;
	bindingDesc.AllowInputAssemblerInputLayout = request.AllowInputAssemblerInputLayout;
	bindingDesc.DebugName = request.BindingLayoutDebugName;
	std::unique_ptr<RenderBindingLayout> bindingLayout =
	    renderHardwareInterface.GetPipelineService().CreateBindingLayout(bindingDesc);
	if (!bindingLayout)
	{
		throw Diagnostics::Error(
		    std::format("Render pass '{}' failed to create its binding layout.", request.PassName));
	}
	return bindingLayout;
}

std::unique_ptr<RenderPipeline> PipelineRuntimeLibrary::CreateGraphicsPipeline(
    RenderHardwareInterface& renderHardwareInterface,
    const GraphicsPipelineDesc& pipelineDesc)
{
	std::unique_ptr<RenderPipeline> pipeline = renderHardwareInterface.GetPipelineService().CreateGraphicsPipeline(pipelineDesc);
	if (!pipeline)
		throw Diagnostics::Error("Graphics pipeline creation failed.");
	return pipeline;
}

std::unique_ptr<RenderPipeline> PipelineRuntimeLibrary::CreateComputePipeline(
    RenderHardwareInterface& renderHardwareInterface,
    const ComputePipelineDesc& pipelineDesc)
{
	std::unique_ptr<RenderPipeline> pipeline = renderHardwareInterface.GetPipelineService().CreateComputePipeline(pipelineDesc);
	if (!pipeline)
		throw Diagnostics::Error("Compute pipeline creation failed.");
	return pipeline;
}
