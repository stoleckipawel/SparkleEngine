#pragma once

#include "FrameGraph/RenderPassRuntime.h"

#include "Config/RenderConfig.h"

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>

#include "RHI/Public/Shaders/DxcShaderCompiler.h"
#include "RHI/Public/Shaders/ShaderCompileResult.h"

#include "Config/DepthConvention.h"
#include "Passes/ComputeClearPass.h"
#include "Passes/ForwardOpaquePass.h"
#include "Passes/ShaderPass.h"
#include "Passes/ShadowOpaquePass.h"
#include "Passes/ShaderSourceDefinition.h"
#include "SceneData/MaterialData.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <array>
#include <memory>

inline void AppendPassParameterLayout(PassParameterLayout& destination, const PassParameterLayout& source)
{
	for (const PassParameterDesc& parameter : source.GetParameters())
	{
		destination.AddParameter(parameter);
	}
}

inline PassParameterLayout BuildShadowOpaqueBindingLayout()
{
	PassParameterLayout layout(ShadowOpaquePass::PassName);
	AppendPassParameterLayout(layout, ShadowOpaquePass::GetParameterMetadata().GetLayout());
	layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
	const bool valid = ValidateShaderPassLayout(layout, ShaderPassKind::Raster, ShadowOpaquePass::PassName);
	assert(valid);
	return layout;
}

inline PassParameterLayout BuildForwardOpaqueBindingLayout()
{
	PassParameterLayout layout(ForwardOpaquePass::PassName);
	layout.Add<ReadTexture>("MaterialTextures", ShaderStageVisibility::Pixel, MaterialTextureSlots::Count);
	AppendPassParameterLayout(layout, ForwardOpaquePass::GetParameterMetadata().GetLayout());
	layout.Add<UniformData<PerObjectVSConstantBufferData>>("PerObjectVS", ShaderStageVisibility::Vertex);
	layout.Add<UniformData<PerObjectPSConstantBufferData>>("PerObjectPS", ShaderStageVisibility::Pixel);
	const bool valid = ValidateShaderPassLayout(layout, ShaderPassKind::Raster, ForwardOpaquePass::PassName);
	assert(valid);
	return layout;
}

template <typename TPass> struct RenderPassRuntimeStorage
{
	std::unique_ptr<RenderBindingLayout> BindingLayout;
	std::unique_ptr<RenderPipelineState> PipelineState;
	std::unique_ptr<ShaderCompileResult> VertexShader;
	std::unique_ptr<ShaderCompileResult> PixelShader;
	std::unique_ptr<ShaderCompileResult> ComputeShader;
};

inline ShaderCompileResult CompileRenderPassShader(const ShaderSourceDefinition& sourceDefinition)
{
	return DxcShaderCompiler::CompileFromAsset(
	    sourceDefinition.GetSourcePath(),
	    sourceDefinition.GetStage(),
	    sourceDefinition.GetEntryPoint());
}

template <typename TPass> struct RenderPassPipelineTraits;

template <> struct RenderPassPipelineTraits<ForwardOpaquePass>
{
	using RuntimeType = RenderPassRuntimeTraits<ForwardOpaquePass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<ForwardOpaquePass>;
	static constexpr std::array<const char*, 7> StableBindingNames =
	    {"PerFrame", "PerView", "ShadowMap0", "ShadowMap1", "ShadowMap2", "ShadowMap3", "SamplerTable"};
	static constexpr std::array<const char*, 3> DrawBindingNames = {"PerObjectVS", "PerObjectPS", "MaterialTextures"};

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, StorageType& storage)
	{
		static const PassParameterLayout bindingLayout = BuildForwardOpaqueBindingLayout();
		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.AllowInputAssemblerInputLayout = true;
		bindingDesc.DebugName = L"ForwardOpaque_RootSignature";
		storage.BindingLayout = rhi.CreateBindingLayout(bindingDesc);

		const ShaderSourceDefinition vertexShader = ForwardOpaquePass::DescribePrimaryViewVertexShader();
		const ShaderSourceDefinition pixelShader = ForwardOpaquePass::DescribePrimaryViewPixelShader();
		ValidateShaderSourceDefinition(vertexShader, ShaderStage::Vertex, ForwardOpaquePass::PassName, "PrimaryViewVertexShader");
		ValidateShaderSourceDefinition(pixelShader, ShaderStage::Pixel, ForwardOpaquePass::PassName, "PrimaryViewPixelShader");

		storage.VertexShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(vertexShader));
		storage.PixelShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(pixelShader));
		GraphicsPipelineStateDesc pipelineDesc{};
		pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderBytecode{storage.VertexShader->GetBytecode().Data, storage.VertexShader->GetBytecode().Size};
		pipelineDesc.PixelShader = RhiShaderBytecode{storage.PixelShader->GetBytecode().Data, storage.PixelShader->GetBytecode().Size};
		pipelineDesc.HasPixelShader = true;
		pipelineDesc.RenderTargetFormats[0] = RenderConfig::BackBufferFormat;
		pipelineDesc.RenderTargetCount = 1;
		pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
		pipelineDesc.DepthTest.DepthEnable = true;
		pipelineDesc.DepthTest.DepthWriteEnable = true;
		pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonFuncEqual();
		pipelineDesc.DebugName = L"ForwardOpaque_PipelineState";
		storage.PipelineState = rhi.CreateGraphicsPipelineState(pipelineDesc);
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

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, StorageType& storage)
	{
		static const PassParameterLayout bindingLayout = BuildShadowOpaqueBindingLayout();
		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.AllowInputAssemblerInputLayout = true;
		bindingDesc.DebugName = L"ShadowOpaque_RootSignature";
		storage.BindingLayout = rhi.CreateBindingLayout(bindingDesc);

		const ShaderSourceDefinition vertexShader = ShadowOpaquePass::DescribeShadowViewVertexShader();
		const ShaderSourceDefinition pixelShader = ShadowOpaquePass::DescribeShadowViewPixelShader();
		ValidateShaderSourceDefinition(vertexShader, ShaderStage::Vertex, ShadowOpaquePass::PassName, "ShadowViewVertexShader");
		ValidateShaderSourceDefinition(pixelShader, ShaderStage::Pixel, ShadowOpaquePass::PassName, "ShadowViewPixelShader");

		storage.VertexShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(vertexShader));
		storage.PixelShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(pixelShader));

		GraphicsPipelineStateDesc pipelineDesc{};
		pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderBytecode{storage.VertexShader->GetBytecode().Data, storage.VertexShader->GetBytecode().Size};
		pipelineDesc.PixelShader = RhiShaderBytecode{storage.PixelShader->GetBytecode().Data, storage.PixelShader->GetBytecode().Size};
		pipelineDesc.HasPixelShader = true;
		pipelineDesc.DepthTest.DepthEnable = true;
		pipelineDesc.DepthTest.DepthWriteEnable = true;
		pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
		pipelineDesc.RenderTargetFormats[0] = RenderConfig::Shadows::ShadowMapFormat;
		pipelineDesc.RenderTargetCount = 1;
		pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
		pipelineDesc.DebugName = L"ShadowDepth_PipelineState";
		storage.PipelineState = rhi.CreateGraphicsPipelineState(pipelineDesc);
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

	static void CreateRuntimeStorage(RenderHardwareInterface& rhi, StorageType& storage)
	{
		RenderBindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &ComputeClearPass::GetParameterLayout();
		bindingDesc.DebugName = L"ComputeClear_RootSignature";
		storage.BindingLayout = rhi.CreateBindingLayout(bindingDesc);

		const ShaderSourceDefinition computeShader = ComputeClearPass::DescribeShader();
		ValidateShaderSourceDefinition(computeShader, ShaderStage::Compute, ComputeClearPass::PassName, "ComputeShader");
		storage.ComputeShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(computeShader));

		ComputePipelineStateDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.ComputeShader = RhiShaderBytecode{storage.ComputeShader->GetBytecode().Data, storage.ComputeShader->GetBytecode().Size};
		pipelineDesc.DebugName = L"ComputeClear_PipelineState";
		storage.PipelineState = rhi.CreateComputePipelineState(pipelineDesc);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};