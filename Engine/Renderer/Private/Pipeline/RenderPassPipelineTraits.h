#pragma once

#include "FrameGraph/RenderPassRuntime.h"
#include "Pipeline/RenderPassShaderRuntime.h"

#include "Config/RenderConfig.h"

#include "Config/DepthConvention.h"
#include "Passes/ComputeClearPass.h"
#include "Passes/ForwardOpaquePass.h"
#include "Passes/ShaderPass.h"
#include "Passes/ShadowOpaquePass.h"

template <typename TPass> struct RenderPassPipelineTraits;

template <> struct RenderPassPipelineTraits<ForwardOpaquePass>
{
	using RuntimeType = RenderPassRuntimeTraits<ForwardOpaquePass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<ForwardOpaquePass>;

	static bool CreateRuntimeStorage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    StorageType& storage,
	    std::string& outErrorMessage)
	{
		return RenderPassShaderRuntime::TryCreateGraphicsRuntime(
		    rhi,
		    shaderPackageCache,
		    RenderPassShaderRuntimeDesc{
		        .PassName = ForwardOpaquePass::PassName,
		        .PackageDeclarationName = "PrimaryViewShaderPackage",
		        .Package = ForwardOpaquePass::DescribePrimaryViewShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Graphics,
		        .AllowInputAssemblerInputLayout = true,
		        .BindingLayoutDebugName = L"ForwardOpaque_RootSignature",
		        .PipelineStateDebugName = L"ForwardOpaque_PipelineState"},
		    storage,
		    [](GraphicsPipelineStateDesc& pipelineDesc)
		    {
			    pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
			    pipelineDesc.RenderTargetFormats[0] = RenderConfig::BackBufferFormat;
			    pipelineDesc.RenderTargetCount = 1;
			    pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
			    pipelineDesc.DepthTest.DepthEnable = true;
			    pipelineDesc.DepthTest.DepthWriteEnable = true;
			    pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonFuncEqual();
		    },
		    outErrorMessage);
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

	static bool CreateRuntimeStorage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    StorageType& storage,
	    std::string& outErrorMessage)
	{
		return RenderPassShaderRuntime::TryCreateGraphicsRuntime(
		    rhi,
		    shaderPackageCache,
		    RenderPassShaderRuntimeDesc{
		        .PassName = ShadowOpaquePass::PassName,
		        .PackageDeclarationName = "ShadowViewShaderPackage",
		        .Package = ShadowOpaquePass::DescribeShadowViewShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Graphics,
		        .AllowInputAssemblerInputLayout = true,
		        .BindingLayoutDebugName = L"ShadowOpaque_RootSignature",
		        .PipelineStateDebugName = L"ShadowDepth_PipelineState"},
		    storage,
		    [](GraphicsPipelineStateDesc& pipelineDesc)
		    {
			    pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
			    pipelineDesc.DepthTest.DepthEnable = true;
			    pipelineDesc.DepthTest.DepthWriteEnable = true;
			    pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
			    pipelineDesc.RenderTargetFormats[0] = RenderConfig::Shadows::ShadowMapFormat;
			    pipelineDesc.RenderTargetCount = 1;
			    pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
		    },
		    outErrorMessage);
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

	static bool CreateRuntimeStorage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    StorageType& storage,
	    std::string& outErrorMessage)
	{
		return RenderPassShaderRuntime::TryCreateComputeRuntime(
		    rhi,
		    shaderPackageCache,
		    RenderPassShaderRuntimeDesc{
		        .PassName = ComputeClearPass::PassName,
		        .PackageDeclarationName = "ComputeShaderPackage",
		        .Package = ComputeClearPass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"ComputeClear_RootSignature",
		        .PipelineStateDebugName = L"ComputeClear_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&) {},
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};