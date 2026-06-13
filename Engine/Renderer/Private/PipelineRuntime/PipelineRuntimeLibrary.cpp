#include "../PCH.h"
#include "PipelineRuntimeLibrary.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Validation/RhiValidation.h"

#include <cassert>
#include <format>

namespace
{
	const std::shared_ptr<spdlog::logger>& GetLogger() noexcept
	{
		static std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer");
		return logger;
	}

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

	PipelineRuntimeKey BuildBasePipelineKey(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const PipelineRuntimePackageRequest& request,
	    const LoadedShaderPackage& shaderPackage,
	    PipelineRuntimeKind kind)
	{
		const RhiCapabilities& capabilities = rhi.GetCapabilities();
		const CookedShaderPackageHeader& header = shaderPackage.GetHeader();
		PipelineRuntimeKey key{};
		key.PassName = std::string(request.PassName);
		key.PackageDeclarationName = std::string(request.PackageDeclarationName);
		key.PackageId = FormatPackageId(request.Package);
		key.BindingLayoutId = FormatBindingLayoutId(request);
		key.Backend = capabilities.BackendApi;
		key.RequiredBinaryFormat = capabilities.RequiredShaderBinaryFormat;
		key.PipelineKind = kind;
		key.ShaderStages = header.DeclaredStages;
		key.RequiredFeatures = request.Package.RequiredFeatures;
		key.PackageFeatures = header.PackageFeatures;
		key.ShaderPackageGeneration = shaderPackageCache.GetGeneration();
		key.ShaderPackageKey = header.ShaderPackageKey;
		key.SourceIdentityHash = header.SourceIdentityHash;
		key.BindingLayoutHash = header.BindingLayoutHash;
		return key;
	}

	void LogPipelineKey(const PipelineRuntimeKey& key)
	{
		SPDLOG_LOGGER_INFO(GetLogger(), "Pipeline runtime key: {}", FormatPipelineRuntimeKey(key));
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
		RhiValidation::ReportContractViolation(
		    "Renderer.Pipeline",
		    outErrorMessage,
		    "build a pass parameter layout before loading the cooked shader package");
		return false;
	}

	const RhiCapabilities& capabilities = rhi.GetCapabilities();
	const CookedShaderBinaryFormat requiredBinaryFormat = capabilities.RequiredShaderBinaryFormat;
	if (!shaderPackageCache.LoadPackage(request.Package, *request.BindingLayout, requiredBinaryFormat, outErrorMessage, outLoadedPackage))
	{
		outErrorMessage = std::format(
		    "Runtime validation rejected cooked shader package '{}' for pass '{}' ({}) with backend='{}' requiredFormat='{}' "
		    "bindingLayout='{}' expectedStages='{}' - {}",
		    FormatPackageId(request.Package),
		    request.PassName,
		    request.PackageDeclarationName,
		    RhiBackendApiToString(capabilities.BackendApi),
		    CookedShaderBinaryFormatToString(requiredBinaryFormat),
		    FormatBindingLayoutId(request),
		    FormatShaderStageMask(request.Package.ExpectedStages),
		    outErrorMessage);
		RhiValidation::ReportContractViolation(
		    "Renderer.Pipeline",
		    outErrorMessage,
		    "recook the shader package, regenerate shader parameter metadata, or fix the pass package declaration so reflection and runtime layout match");
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
		RhiValidation::ReportContractViolation(
		    "Renderer.Pipeline",
		    outErrorMessage,
		    "disable the pass, select a non-ray-tracing permutation, or implement and truthfully report backend ray tracing support");
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
		RhiValidation::ReportContractViolation(
		    "Renderer.Pipeline",
		    outErrorMessage,
		    "disable the pass, select a non-ray-query permutation, or implement and truthfully report backend inline ray query support");
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
	return rhi.CreateBindingLayout(bindingDesc);
}

std::unique_ptr<RenderPipelineState> PipelineRuntimeLibrary::CreateGraphicsPipelineState(
    RenderHardwareInterface& rhi,
    CookedShaderPackageCache& shaderPackageCache,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage,
    const GraphicsPipelineStateDesc& pipelineDesc)
{
	const PipelineRuntimeKey key = BuildGraphicsPipelineKey(rhi, shaderPackageCache, request, shaderPackage, pipelineDesc);
	LogPipelineKey(key);
	return rhi.CreateGraphicsPipelineState(pipelineDesc);
}

std::unique_ptr<RenderPipelineState> PipelineRuntimeLibrary::CreateComputePipelineState(
    RenderHardwareInterface& rhi,
    CookedShaderPackageCache& shaderPackageCache,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage,
    const ComputePipelineStateDesc& pipelineDesc)
{
	const PipelineRuntimeKey key = BuildComputePipelineKey(rhi, shaderPackageCache, request, shaderPackage);
	LogPipelineKey(key);
	return rhi.CreateComputePipelineState(pipelineDesc);
}

PipelineRuntimeKey PipelineRuntimeLibrary::BuildGraphicsPipelineKey(
    RenderHardwareInterface& rhi,
    CookedShaderPackageCache& shaderPackageCache,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage,
    const GraphicsPipelineStateDesc& pipelineDesc)
{
	PipelineRuntimeKey key = BuildBasePipelineKey(rhi, shaderPackageCache, request, shaderPackage, PipelineRuntimeKind::Graphics);
	key.VertexLayout = pipelineDesc.VertexLayout;
	key.HasPixelShader = pipelineDesc.PixelShader.IsValid();
	key.RenderWireframe = pipelineDesc.RenderWireframe;
	key.CullMode = pipelineDesc.CullMode;
	key.FrontFaceWinding = pipelineDesc.FrontFaceWinding;
	key.DepthTest = pipelineDesc.DepthTest;
	key.StencilTest = pipelineDesc.StencilTest;
	key.RenderTargetFormats = pipelineDesc.RenderTargetFormats;
	key.RenderTargetCount = pipelineDesc.RenderTargetCount;
	key.DepthStencilFormat = pipelineDesc.DepthStencilFormat;
	return key;
}

PipelineRuntimeKey PipelineRuntimeLibrary::BuildComputePipelineKey(
    RenderHardwareInterface& rhi,
    CookedShaderPackageCache& shaderPackageCache,
    const PipelineRuntimePackageRequest& request,
    const LoadedShaderPackage& shaderPackage)
{
	return BuildBasePipelineKey(rhi, shaderPackageCache, request, shaderPackage, PipelineRuntimeKind::Compute);
}
