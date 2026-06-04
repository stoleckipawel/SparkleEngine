#pragma once

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/CookedShaderPackageCache.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutBuilder.h"
#include "RHI/Public/Validation/RhiValidation.h"

#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

enum class RenderPassShaderPipelineKind
{
	Graphics,
	Compute,
};

struct RenderPassShaderRuntimeDesc final
{
	std::string_view PassName;
	std::string_view PackageDeclarationName;
	ShaderPackageDefinition Package;
	RenderPassShaderPipelineKind PipelineKind = RenderPassShaderPipelineKind::Graphics;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* BindingLayoutDebugName = L"RenderPass_BindingLayout";
	const wchar_t* PipelineStateDebugName = L"RenderPass_PipelineState";
};

struct RenderPassShaderRuntimeStorage
{
	PassParameterLayout BindingLayoutDefinition;
	std::unique_ptr<RenderBindingLayout> BindingLayout;
	std::unique_ptr<RenderPipelineState> PipelineState;
	std::unique_ptr<RenderPipelineState> WireframePipelineState;
	std::unique_ptr<RenderPipelineState> TwoSidedPipelineState;
	const LoadedShaderPackage* ShaderPackage = nullptr;
};

class RenderPassShaderRuntime final
{
  public:
	template <typename ConfigurePipelineState>
	static void CreateGraphicsRuntime(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipelineState configurePipelineState)
	{
		std::string errorMessage;
		if (!TryCreateGraphicsRuntime(rhi, shaderPackageCache, desc, storage, configurePipelineState, errorMessage))
		{
			Diagnostics::Fail(GetLogger(), __FILE__, __LINE__, errorMessage);
		}
	}

	template <typename ConfigurePipelineState>
	static bool TryCreateGraphicsRuntime(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipelineState configurePipelineState,
	    std::string& outErrorMessage)
	{
		if (!ValidatePipelineKind(desc, RenderPassShaderPipelineKind::Graphics, outErrorMessage) ||
		    !ValidateExpectedStages(desc, outErrorMessage) || !BuildBindingLayout(desc, storage.BindingLayoutDefinition, outErrorMessage) ||
		    !LoadShaderPackage(rhi, shaderPackageCache, desc, storage.BindingLayoutDefinition, storage.ShaderPackage, outErrorMessage) ||
		    !ValidatePackageCapabilities(rhi, desc, *storage.ShaderPackage, outErrorMessage))
		{
			return false;
		}

		storage.BindingLayout = CreateBindingLayout(rhi, desc, storage.BindingLayoutDefinition, *storage.ShaderPackage);

		GraphicsPipelineStateDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Vertex};
		if (HasAnyShaderStageMask(desc.Package.ExpectedStages, ShaderStageMask::Pixel))
		{
			pipelineDesc.PixelShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Pixel};
		}
		pipelineDesc.DebugName = desc.PipelineStateDebugName;
		configurePipelineState(pipelineDesc);
		storage.PipelineState = rhi.CreateGraphicsPipelineState(pipelineDesc);
		storage.WireframePipelineState.reset();
		storage.TwoSidedPipelineState.reset();
		if (desc.AllowInputAssemblerInputLayout && !pipelineDesc.RenderWireframe)
		{
			GraphicsPipelineStateDesc twoSidedPipelineDesc = pipelineDesc;
			twoSidedPipelineDesc.CullMode = ERhiCullMode::None;
			storage.TwoSidedPipelineState = rhi.CreateGraphicsPipelineState(twoSidedPipelineDesc);
			if (!storage.TwoSidedPipelineState)
			{
				outErrorMessage = std::format(
				    "Render pass '{}' failed to create its two-sided graphics pipeline state for package '{}'",
				    desc.PassName,
				    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>");
				return false;
			}

			GraphicsPipelineStateDesc wireframePipelineDesc = pipelineDesc;
			wireframePipelineDesc.RenderWireframe = true;
			wireframePipelineDesc.CullMode = ERhiCullMode::None;
			storage.WireframePipelineState = rhi.CreateGraphicsPipelineState(wireframePipelineDesc);
			if (!storage.WireframePipelineState)
			{
				outErrorMessage = std::format(
				    "Render pass '{}' failed to create its wireframe graphics pipeline state for package '{}'",
				    desc.PassName,
				    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>");
				return false;
			}
		}

		LogRuntimeReady(rhi, desc);
		outErrorMessage.clear();
		return true;
	}

	template <typename ConfigurePipelineState>
	static void CreateComputeRuntime(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipelineState configurePipelineState)
	{
		std::string errorMessage;
		if (!TryCreateComputeRuntime(rhi, shaderPackageCache, desc, storage, configurePipelineState, errorMessage))
		{
			Diagnostics::Fail(GetLogger(), __FILE__, __LINE__, errorMessage);
		}
	}

	template <typename ConfigurePipelineState>
	static bool TryCreateComputeRuntime(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipelineState configurePipelineState,
	    std::string& outErrorMessage)
	{
		if (!ValidatePipelineKind(desc, RenderPassShaderPipelineKind::Compute, outErrorMessage) ||
		    !ValidateExpectedStages(desc, outErrorMessage) || !BuildBindingLayout(desc, storage.BindingLayoutDefinition, outErrorMessage) ||
		    !LoadShaderPackage(rhi, shaderPackageCache, desc, storage.BindingLayoutDefinition, storage.ShaderPackage, outErrorMessage) ||
		    !ValidatePackageCapabilities(rhi, desc, *storage.ShaderPackage, outErrorMessage))
		{
			return false;
		}

		storage.BindingLayout = CreateBindingLayout(rhi, desc, storage.BindingLayoutDefinition, *storage.ShaderPackage);

		ComputePipelineStateDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.ComputeShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Compute};
		pipelineDesc.DebugName = desc.PipelineStateDebugName;
		configurePipelineState(pipelineDesc);
		storage.PipelineState = rhi.CreateComputePipelineState(pipelineDesc);

		LogRuntimeReady(rhi, desc);
		outErrorMessage.clear();
		return true;
	}

  private:
	static const std::shared_ptr<spdlog::logger>& GetLogger() noexcept
	{
		static std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer");
		return logger;
	}

	static const char* FormatPipelineKind(RenderPassShaderPipelineKind kind) noexcept
	{
		switch (kind)
		{
			case RenderPassShaderPipelineKind::Graphics:
				return "graphics";
			case RenderPassShaderPipelineKind::Compute:
				return "compute";
		}

		return "unknown";
	}

	static bool ValidatePipelineKind(
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderPipelineKind expectedKind,
	    std::string& outErrorMessage)
	{
		if (desc.PipelineKind == expectedKind)
		{
			return true;
		}

		outErrorMessage = std::format(
		    "Render pass '{}' requested a {} shader runtime through the {} facade path for package '{}'",
		    desc.PassName,
		    FormatPipelineKind(desc.PipelineKind),
		    FormatPipelineKind(expectedKind),
		    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>");
		return false;
	}

	static bool ValidateExpectedStages(const RenderPassShaderRuntimeDesc& desc, std::string& outErrorMessage)
	{
		const bool hasVertex = HasAnyShaderStageMask(desc.Package.ExpectedStages, ShaderStageMask::Vertex);
		const bool hasCompute = HasAnyShaderStageMask(desc.Package.ExpectedStages, ShaderStageMask::Compute);
		const bool isGraphics = desc.PipelineKind == RenderPassShaderPipelineKind::Graphics;
		const bool valid = isGraphics ? (hasVertex && !hasCompute) : (hasCompute && !hasVertex);
		if (valid)
		{
			return true;
		}

		outErrorMessage = std::format(
		    "Render pass '{}' declares conflicting expected stages '{}' for {} shader package '{}' ({})",
		    desc.PassName,
		    FormatShaderStageMask(desc.Package.ExpectedStages),
		    FormatPipelineKind(desc.PipelineKind),
		    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>",
		    desc.PackageDeclarationName);
		return false;
	}

	static bool BuildBindingLayout(
	    const RenderPassShaderRuntimeDesc& desc,
	    PassParameterLayout& outBindingLayout,
	    std::string& outErrorMessage)
	{
		if (!desc.Package.IsValid())
		{
			outErrorMessage = std::format(
			    "Render pass '{}' declares an invalid cooked shader package for '{}'",
			    desc.PassName,
			    desc.PackageDeclarationName);
			return false;
		}

		PassParameterLayout bindingLayout;
		if (!BuildRegisteredShaderPackageLayout(desc.Package.PackageId, bindingLayout, outErrorMessage))
		{
			outErrorMessage = std::format(
			    "Failed to build generated shader package layout '{}' for pass '{}' ({}) - {}",
			    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>",
			    desc.PassName,
			    desc.PackageDeclarationName,
			    outErrorMessage);
			return false;
		}

		outBindingLayout = std::move(bindingLayout);
		outErrorMessage.clear();
		return true;
	}

	static bool LoadShaderPackage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    const PassParameterLayout& bindingLayout,
	    const LoadedShaderPackage*& outLoadedPackage,
	    std::string& outErrorMessage)
	{
		outLoadedPackage = nullptr;
		const RhiCapabilities& capabilities = rhi.GetCapabilities();
		const CookedShaderBinaryFormat requiredBinaryFormat = capabilities.RequiredShaderBinaryFormat;
		if (!shaderPackageCache
		         .LoadPackage(desc.Package, bindingLayout, requiredBinaryFormat, outErrorMessage, outLoadedPackage))
		{
			const std::string bindingLayoutLabel =
			    desc.Package.BindingLayoutId != nullptr ? std::string(desc.Package.BindingLayoutId) : bindingLayout.GetDebugName();
			outErrorMessage = std::format(
			    "Runtime validation rejected cooked shader package '{}' for pass '{}' ({}) with backend='{}' requiredFormat='{}' "
			    "bindingLayout='{}' expectedStages='{}' - "
			    "{}",
			    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>",
			    desc.PassName,
			    desc.PackageDeclarationName,
			    RhiBackendApiToString(capabilities.BackendApi),
			    CookedShaderBinaryFormatToString(requiredBinaryFormat),
			    bindingLayoutLabel,
			    FormatShaderStageMask(desc.Package.ExpectedStages),
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

	static bool ValidatePackageCapabilities(
	    RenderHardwareInterface& rhi,
	    const RenderPassShaderRuntimeDesc& desc,
	    const LoadedShaderPackage& shaderPackage,
	    std::string& outErrorMessage)
	{
		const RhiCapabilities& capabilities = rhi.GetCapabilities();
		const CookedShaderPackageFeatureFlags packageFeatures = shaderPackage.GetHeader().PackageFeatures;
		if (HasCookedShaderPackageFeature(packageFeatures, CookedShaderPackageFeatureFlags::UsesAccelerationStructure) &&
		    !capabilities.RayTracing.SupportsRayTracing)
		{
			outErrorMessage = std::format(
			    "Render pass '{}' package '{}' requires acceleration-structure bindings, but backend '{}' reports ray tracing unsupported",
			    desc.PassName,
			    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>",
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
			    desc.PassName,
			    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>",
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

	static std::unique_ptr<RenderBindingLayout> CreateBindingLayout(
	    RenderHardwareInterface& rhi,
	    const RenderPassShaderRuntimeDesc& desc,
	    const PassParameterLayout& bindingLayout,
	    const LoadedShaderPackage& shaderPackage)
	{
		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.ShaderPackage = &shaderPackage;
		bindingDesc.AllowInputAssemblerInputLayout = desc.AllowInputAssemblerInputLayout;
		bindingDesc.DebugName = desc.BindingLayoutDebugName;
		return rhi.CreateBindingLayout(bindingDesc);
	}

	static void LogRuntimeReady(RenderHardwareInterface& rhi, const RenderPassShaderRuntimeDesc& desc)
	{
		const RhiCapabilities& capabilities = rhi.GetCapabilities();
		SPDLOG_LOGGER_INFO(
		    GetLogger(),
		    "Cooked shader runtime ready: pass='{}' pipeline='{}' backend='{}' requiredFormat='{}' package='{}' bindingLayout='{}' "
		    "expectedStages='{}'",
		    desc.PassName,
		    FormatPipelineKind(desc.PipelineKind),
		    RhiBackendApiToString(capabilities.BackendApi),
		    CookedShaderBinaryFormatToString(capabilities.RequiredShaderBinaryFormat),
		    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>",
		    desc.Package.BindingLayoutId != nullptr ? desc.Package.BindingLayoutId : "<null>",
		    FormatShaderStageMask(desc.Package.ExpectedStages));
	}
};
