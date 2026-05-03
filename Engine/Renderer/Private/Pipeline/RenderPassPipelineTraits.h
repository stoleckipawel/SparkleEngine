#pragma once

#include "FrameGraph/RenderPassRuntime.h"
#include "Pipeline/RenderPassShaderRuntime.h"

#include "Config/RenderConfig.h"

#include "Config/DepthConvention.h"
#include "Passes/ComputeClearPass.h"
#include "Passes/DeferredLightingPass.h"
#include "Passes/GBufferPass.h"
#include "Passes/ShaderPass.h"

template <typename TPass> struct RenderPassPipelineTraits;

template <> struct RenderPassPipelineTraits<GBufferPass>
{
	using RuntimeType = RenderPassRuntimeTraits<GBufferPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<GBufferPass>;

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
		        .PassName = GBufferPass::PassName,
		        .PackageDeclarationName = "GBufferShaderPackage",
		        .Package = GBufferPass::DescribeGBufferShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Graphics,
		        .AllowInputAssemblerInputLayout = true,
		        .BindingLayoutDebugName = L"GBuffer_RootSignature",
		        .PipelineStateDebugName = L"GBuffer_PipelineState"},
		    storage,
		    [](GraphicsPipelineStateDesc& pipelineDesc)
		    {
			    pipelineDesc.VertexLayout = RhiVertexLayoutKind::StaticMesh;
			    pipelineDesc.RenderTargetFormats[0] = RenderConfig::GBuffer::BaseColorFormat;
			    pipelineDesc.RenderTargetFormats[1] = RenderConfig::GBuffer::NormalFormat;
			    pipelineDesc.RenderTargetFormats[2] = RenderConfig::GBuffer::MaterialFormat;
			    pipelineDesc.RenderTargetFormats[3] = RenderConfig::GBuffer::EmissiveFormat;
			    pipelineDesc.RenderTargetFormats[4] = RenderConfig::GBuffer::DeviceZFormat;
			    pipelineDesc.RenderTargetCount = 5;
			    pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
			    pipelineDesc.DepthTest.DepthEnable = true;
			    pipelineDesc.DepthTest.DepthWriteEnable = true;
			    pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
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

template <> struct RenderPassPipelineTraits<DeferredLightingPass>
{
	using RuntimeType = RenderPassRuntimeTraits<DeferredLightingPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<DeferredLightingPass>;

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
		        .PassName = DeferredLightingPass::PassName,
		        .PackageDeclarationName = "DeferredLightingShaderPackage",
		        .Package = DeferredLightingPass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"DeferredLighting_RootSignature",
		        .PipelineStateDebugName = L"DeferredLighting_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&) {},
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};
