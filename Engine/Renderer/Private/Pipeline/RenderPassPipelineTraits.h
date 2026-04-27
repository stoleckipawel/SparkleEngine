#pragma once

#include "FrameGraph/RenderPassRuntime.h"

#include "Config/RenderConfig.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

#include "Config/DepthConvention.h"
#include "Passes/ComputeClearPass.h"
#include "Passes/ForwardOpaquePass.h"
#include "Passes/ShaderPass.h"
#include "Passes/ShadowOpaquePass.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/CookedShaderPackageCache.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutBuilder.h"

#include <array>
#include <cassert>
#include <format>
#include <memory>

inline std::string FormatShaderStageMask(ShaderStageMask mask)
{
	if (mask == ShaderStageMask::None)
	{
		return "None";
	}

	struct StageLabel
	{
		ShaderStageMask Mask;
		const char* Label;
	};

	static constexpr std::array<StageLabel, 6> kStageLabels = {{
	    {ShaderStageMask::Vertex, "Vertex"},
	    {ShaderStageMask::Pixel, "Pixel"},
	    {ShaderStageMask::Geometry, "Geometry"},
	    {ShaderStageMask::Hull, "Hull"},
	    {ShaderStageMask::Domain, "Domain"},
	    {ShaderStageMask::Compute, "Compute"},
	}};

	std::string result;
	for (const StageLabel& stageLabel : kStageLabels)
	{
		if (!HasAnyShaderStageMask(mask, stageLabel.Mask))
		{
			continue;
		}

		if (!result.empty())
		{
			result += '|';
		}

		result += stageLabel.Label;
	}

	return result.empty() ? "None" : result;
}

inline const std::shared_ptr<spdlog::logger>& GetRendererPipelineLogger() noexcept
{
	static std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer");
	return logger;
}

inline void LogCookedShaderRuntimeReady(
	std::string_view passName,
	std::string_view pipelineKind,
	const ShaderPackageDefinition& definition)
{
	SPDLOG_LOGGER_INFO(
	    GetRendererPipelineLogger(),
	    "Cooked shader runtime ready: pass='{}' pipeline='{}' package='{}' variant='{}' bindingLayout='{}' expectedStages='{}'",
	    passName,
	    pipelineKind,
	    definition.PackageId != nullptr ? definition.PackageId : "<null>",
	    definition.VariantId != nullptr ? definition.VariantId : "<null>",
	    definition.BindingLayoutId != nullptr ? definition.BindingLayoutId : "<null>",
	    FormatShaderStageMask(definition.ExpectedStages));
}

inline PassParameterLayout BuildRenderPassShaderPackageLayout(
    const ShaderPackageDefinition& definition,
    std::string_view passName,
    std::string_view declarationName)
{
	if (!definition.IsValid())
	{
		Diagnostics::Fail(
		    GetRendererPipelineLogger(),
		    __FILE__,
		    __LINE__,
		    std::format("Render pass '{}' declares an invalid cooked shader package for '{}'", passName, declarationName));
		return PassParameterLayout("Invalid");
	}

	PassParameterLayout bindingLayout;
	std::string errorMessage;
	if (!BuildRegisteredShaderPackageLayout(definition.PackageId, bindingLayout, errorMessage))
	{
		Diagnostics::Fail(
		    GetRendererPipelineLogger(),
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Failed to build generated shader package layout '{}' for pass '{}' ({}) - {}",
		        definition.PackageId != nullptr ? definition.PackageId : "<null>",
		        passName,
		        declarationName,
		        errorMessage));
		return PassParameterLayout("Invalid");
	}

	return bindingLayout;
}

template <typename TPass> struct RenderPassRuntimeStorage
{
	std::unique_ptr<RenderBindingLayout> BindingLayout;
	std::unique_ptr<RenderPipelineState> PipelineState;
	const LoadedShaderPackage* ShaderPackage = nullptr;
};

inline const LoadedShaderPackage& LoadRenderPassShaderPackage(
	RenderHardwareInterface& rhi,
    CookedShaderPackageCache& shaderPackageCache,
    const ShaderPackageDefinition& definition,
    const PassParameterLayout& bindingLayout,
    std::string_view passName,
    std::string_view declarationName)
{
	if (!definition.IsValid())
	{
		Diagnostics::Fail(
		    GetRendererPipelineLogger(),
		    __FILE__,
		    __LINE__,
		    std::format("Render pass '{}' declares an invalid cooked shader package for '{}'", passName, declarationName));
	}

	const LoadedShaderPackage* loadedPackage = nullptr;
	std::string errorMessage;
	if (!shaderPackageCache.LoadPackage(definition, bindingLayout, rhi.GetRequiredShaderBinaryFormat(), errorMessage, loadedPackage))
	{
		const std::string bindingLayoutLabel =
		    definition.BindingLayoutId != nullptr ? std::string(definition.BindingLayoutId) : bindingLayout.GetDebugName();
		Diagnostics::Fail(
		    GetRendererPipelineLogger(),
		    __FILE__,
		    __LINE__,
		    std::format(
		        "Failed to load cooked shader package '{}' variant '{}' for pass '{}' ({}) with bindingLayout='{}' expectedStages='{}' - {}",
		        definition.PackageId != nullptr ? definition.PackageId : "<null>",
		        definition.VariantId != nullptr ? definition.VariantId : "<null>",
		        passName,
		        declarationName,
		        bindingLayoutLabel,
		        FormatShaderStageMask(definition.ExpectedStages),
		        errorMessage));
	}

	assert(loadedPackage != nullptr);
	return *loadedPackage;
}

template <typename TPass> struct RenderPassPipelineTraits;

template <> struct RenderPassPipelineTraits<ForwardOpaquePass>
{
	using RuntimeType = RenderPassRuntimeTraits<ForwardOpaquePass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<ForwardOpaquePass>;

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, CookedShaderPackageCache& shaderPackageCache, StorageType& storage)
	{
		const ShaderPackageDefinition shaderPackage = ForwardOpaquePass::DescribePrimaryViewShaderPackage();
		static const PassParameterLayout bindingLayout =
		    BuildRenderPassShaderPackageLayout(shaderPackage, ForwardOpaquePass::PassName, "PrimaryViewShaderPackage");
		storage.ShaderPackage = &LoadRenderPassShaderPackage(
		    rhi,
		    shaderPackageCache,
		    shaderPackage,
		    bindingLayout,
		    ForwardOpaquePass::PassName,
		    "PrimaryViewShaderPackage");

		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.ShaderPackage = storage.ShaderPackage;
		bindingDesc.AllowInputAssemblerInputLayout = true;
		bindingDesc.DebugName = L"ForwardOpaque_RootSignature";
		storage.BindingLayout = rhi.CreateBindingLayout(bindingDesc);
		GraphicsPipelineStateDesc pipelineDesc{};
		pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Vertex};
		pipelineDesc.PixelShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Pixel};
		pipelineDesc.RenderTargetFormats[0] = RenderConfig::BackBufferFormat;
		pipelineDesc.RenderTargetCount = 1;
		pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
		pipelineDesc.DepthTest.DepthEnable = true;
		pipelineDesc.DepthTest.DepthWriteEnable = true;
		pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonFuncEqual();
		pipelineDesc.DebugName = L"ForwardOpaque_PipelineState";
		storage.PipelineState = rhi.CreateGraphicsPipelineState(pipelineDesc);
		LogCookedShaderRuntimeReady(ForwardOpaquePass::PassName, "graphics", shaderPackage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<ShadowOpaquePass>
{
	using RuntimeType = RenderPassRuntimeTraits<ShadowOpaquePass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<ShadowOpaquePass>;

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, CookedShaderPackageCache& shaderPackageCache, StorageType& storage)
	{
		const ShaderPackageDefinition shaderPackage = ShadowOpaquePass::DescribeShadowViewShaderPackage();
		static const PassParameterLayout bindingLayout =
		    BuildRenderPassShaderPackageLayout(shaderPackage, ShadowOpaquePass::PassName, "ShadowViewShaderPackage");
		storage.ShaderPackage = &LoadRenderPassShaderPackage(
		    rhi,
		    shaderPackageCache,
		    shaderPackage,
		    bindingLayout,
		    ShadowOpaquePass::PassName,
		    "ShadowViewShaderPackage");

		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.ShaderPackage = storage.ShaderPackage;
		bindingDesc.AllowInputAssemblerInputLayout = true;
		bindingDesc.DebugName = L"ShadowOpaque_RootSignature";
		storage.BindingLayout = rhi.CreateBindingLayout(bindingDesc);

		GraphicsPipelineStateDesc pipelineDesc{};
		pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Vertex};
		pipelineDesc.PixelShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Pixel};
		pipelineDesc.DepthTest.DepthEnable = true;
		pipelineDesc.DepthTest.DepthWriteEnable = true;
		pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
		pipelineDesc.RenderTargetFormats[0] = RenderConfig::Shadows::ShadowMapFormat;
		pipelineDesc.RenderTargetCount = 1;
		pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
		pipelineDesc.DebugName = L"ShadowDepth_PipelineState";
		storage.PipelineState = rhi.CreateGraphicsPipelineState(pipelineDesc);
		LogCookedShaderRuntimeReady(ShadowOpaquePass::PassName, "graphics", shaderPackage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<ComputeClearPass>
{
	using RuntimeType = RenderPassRuntimeTraits<ComputeClearPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<ComputeClearPass>;

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, CookedShaderPackageCache& shaderPackageCache, StorageType& storage)
	{
		const ShaderPackageDefinition shaderPackage = ComputeClearPass::DescribeShaderPackage();
		static const PassParameterLayout bindingLayout =
		    BuildRenderPassShaderPackageLayout(shaderPackage, ComputeClearPass::PassName, "ComputeShaderPackage");
		storage.ShaderPackage = &LoadRenderPassShaderPackage(
		    rhi,
		    shaderPackageCache,
		    shaderPackage,
		    bindingLayout,
		    ComputeClearPass::PassName,
		    "ComputeShaderPackage");

		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.ShaderPackage = storage.ShaderPackage;
		bindingDesc.DebugName = L"ComputeClear_RootSignature";
		storage.BindingLayout = rhi.CreateBindingLayout(bindingDesc);

		ComputePipelineStateDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.ComputeShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Compute};
		pipelineDesc.DebugName = L"ComputeClear_PipelineState";
		storage.PipelineState = rhi.CreateComputePipelineState(pipelineDesc);
		LogCookedShaderRuntimeReady(ComputeClearPass::PassName, "compute", shaderPackage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};