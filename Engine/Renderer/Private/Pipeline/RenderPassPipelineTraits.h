#pragma once

#include "FrameGraph/RenderPassRuntime.h"

#include "Config/RenderConfig.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/Pipeline/D3D12VertexLayout.h"
#include "D3D12/Shaders/DxcShaderCompiler.h"
#include "D3D12/Shaders/ShaderCompileResult.h"

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
	std::unique_ptr<D3D12BindingLayout> BindingLayout;
	std::unique_ptr<D3D12PipelineState> PipelineState;
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

	static void CreateRuntimeStorage(D3D12Rhi& rhi, StorageType& storage)
	{
		static const PassParameterLayout bindingLayout = BuildForwardOpaqueBindingLayout();
		D3D12BindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		bindingDesc.DebugName = L"ForwardOpaque_RootSignature";
		storage.BindingLayout = D3D12BindingLayoutCompiler::Compile(rhi, bindingDesc);

		const ShaderSourceDefinition vertexShader = ForwardOpaquePass::DescribePrimaryViewVertexShader();
		const ShaderSourceDefinition pixelShader = ForwardOpaquePass::DescribePrimaryViewPixelShader();
		ValidateShaderSourceDefinition(vertexShader, ShaderStage::Vertex, ForwardOpaquePass::PassName, "PrimaryViewVertexShader");
		ValidateShaderSourceDefinition(pixelShader, ShaderStage::Pixel, ForwardOpaquePass::PassName, "PrimaryViewPixelShader");

		storage.VertexShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(vertexShader));
		storage.PixelShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(pixelShader));
		storage.PipelineState = std::make_unique<D3D12PipelineState>(
		    rhi,
		    D3D12VertexLayout::GetStaticMeshLayout(),
		    storage.BindingLayout->GetRootSignature(),
		    storage.VertexShader->GetBytecode(),
		    storage.PixelShader->GetBytecode());
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

	static void CreateRuntimeStorage(D3D12Rhi& rhi, StorageType& storage)
	{
		static const PassParameterLayout bindingLayout = BuildShadowOpaqueBindingLayout();
		D3D12BindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &bindingLayout;
		bindingDesc.RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		bindingDesc.DebugName = L"ShadowOpaque_RootSignature";
		storage.BindingLayout = D3D12BindingLayoutCompiler::Compile(rhi, bindingDesc);

		const ShaderSourceDefinition vertexShader = ShadowOpaquePass::DescribeShadowViewVertexShader();
		const ShaderSourceDefinition pixelShader = ShadowOpaquePass::DescribeShadowViewPixelShader();
		ValidateShaderSourceDefinition(vertexShader, ShaderStage::Vertex, ShadowOpaquePass::PassName, "ShadowViewVertexShader");
		ValidateShaderSourceDefinition(pixelShader, ShaderStage::Pixel, ShadowOpaquePass::PassName, "ShadowViewPixelShader");

		storage.VertexShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(vertexShader));
		storage.PixelShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(pixelShader));

		GraphicsPipelineStateDesc pipelineDesc{};
		pipelineDesc.VertexLayout = D3D12VertexLayout::GetStaticMeshLayout();
		pipelineDesc.RootSignature = &storage.BindingLayout->GetRootSignature();
		pipelineDesc.VertexShader = storage.VertexShader->GetBytecode();
		pipelineDesc.PixelShader = storage.PixelShader->GetBytecode();
		pipelineDesc.HasPixelShader = true;
		pipelineDesc.DepthTest.DepthEnable = true;
		pipelineDesc.DepthTest.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
		pipelineDesc.RenderTargetFormats[0] = RenderConfig::Shadows::ShadowMapFormat;
		pipelineDesc.RenderTargetCount = 1;
		pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
		pipelineDesc.DebugName = L"ShadowDepth_PipelineState";
		storage.PipelineState = std::make_unique<D3D12PipelineState>(rhi, pipelineDesc);
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

	static void CreateRuntimeStorage(D3D12Rhi& rhi, StorageType& storage)
	{
		D3D12BindingLayoutCompileDesc bindingDesc{};
		bindingDesc.ParameterLayout = &ComputeClearPass::GetParameterLayout();
		bindingDesc.DebugName = L"ComputeClear_RootSignature";
		storage.BindingLayout = D3D12BindingLayoutCompiler::Compile(rhi, bindingDesc);

		const ShaderSourceDefinition computeShader = ComputeClearPass::DescribeShader();
		ValidateShaderSourceDefinition(computeShader, ShaderStage::Compute, ComputeClearPass::PassName, "ComputeShader");
		storage.ComputeShader = std::make_unique<ShaderCompileResult>(CompileRenderPassShader(computeShader));

		ComputePipelineStateDesc pipelineDesc{};
		pipelineDesc.RootSignature = &storage.BindingLayout->GetRootSignature();
		pipelineDesc.ComputeShader = storage.ComputeShader->GetBytecode();
		pipelineDesc.DebugName = L"ComputeClear_PipelineState";
		storage.PipelineState = std::make_unique<D3D12PipelineState>(rhi, pipelineDesc);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};