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
	static std::shared_ptr<spdlog::logger> logger = Engine::Logging::GetOrCreateLogger("Renderer");
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

inline void AppendPassParameterLayout(PassParameterLayout& destination, const PassParameterLayout& source)
{
	for (const PassParameterDesc& parameter : source.GetParameters())
	{
		destination.AddParameter(parameter);
	}
}

inline PassParameterLayout BuildShadowOpaqueBindingLayout()
{
	PassParameterLayout layout = ShadowOpaquePass::GetParameterMetadata().GetLayout();
	layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
	const bool valid = ValidateShaderPassLayout(layout, ShaderPassKind::Raster, ShadowOpaquePass::PassName);
	assert(valid);
	return layout;
}

inline PassParameterLayout BuildForwardOpaqueBindingLayout()
{
	PassParameterLayout layout = ForwardOpaquePass::GetParameterMetadata().GetLayout();
	layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
	layout.Add<UniformData<PerObjectPSConstantBufferData>>("PerObjectPS", ShaderStageVisibility::Pixel);
	layout.Add<ReadTexture>("MaterialTextures", ShaderStageVisibility::Pixel, 5u);
	const bool valid = ValidateShaderPassLayout(layout, ShaderPassKind::Raster, ForwardOpaquePass::PassName);
	assert(valid);
	return layout;
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
		Engine::Diagnostics::Fail(
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
		Engine::Diagnostics::Fail(
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
	static constexpr std::array<const char*, 7> StableBindingNames =
	    {"PerFrame", "PerView", "ShadowMap0", "ShadowMap1", "ShadowMap2", "ShadowMap3", "SamplerTable"};
	static constexpr std::array<const char*, 3> DrawBindingNames = {"PerObjectVS", "PerObjectPS", "MaterialTextures"};

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, CookedShaderPackageCache& shaderPackageCache, StorageType& storage)
	{
		static const PassParameterLayout bindingLayout = BuildForwardOpaqueBindingLayout();
		const ShaderPackageDefinition shaderPackage = ForwardOpaquePass::DescribePrimaryViewShaderPackage();
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
	static constexpr std::array<const char*, 2> StableBindingNames = {"PerFrame", "PerView"};
	static constexpr std::array<const char*, 1> DrawBindingNames = {"PerObjectVS"};

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, CookedShaderPackageCache& shaderPackageCache, StorageType& storage)
	{
		static const PassParameterLayout bindingLayout = BuildShadowOpaqueBindingLayout();
		const ShaderPackageDefinition shaderPackage = ShadowOpaquePass::DescribeShadowViewShaderPackage();
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
		storage.ShaderPackage = &LoadRenderPassShaderPackage(
		    rhi,
		    shaderPackageCache,
		    shaderPackage,
		    ComputeClearPass::GetParameterLayout(),
		    ComputeClearPass::PassName,
		    "ComputeShaderPackage");

		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &ComputeClearPass::GetParameterLayout();
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