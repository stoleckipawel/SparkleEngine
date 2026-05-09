#pragma once

#include "FrameGraph/RenderPassRuntime.h"
#include "Pipeline/RenderPassShaderRuntime.h"

#include "Config/RenderConfig.h"

#include "Config/DepthConvention.h"
#include "Passes/ComputeClearPass.h"
#include "Passes/DirectLightingPass.h"
#include "Passes/GBufferPass.h"
#include "Passes/IndirectLightingPass.h"
#include "Passes/LightingCompositePass.h"
#include "Passes/ShaderPass.h"
#include "Passes/SkyPass.h"
#include "Passes/VisualizeBuffersPass.h"

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
			    pipelineDesc.RenderTargetFormats[4] = RenderConfig::GBuffer::SubsurfaceFormat;
			    pipelineDesc.RenderTargetFormats[5] = RenderConfig::GBuffer::DeviceZFormat;
			    pipelineDesc.RenderTargetCount = 6;
			    pipelineDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
			    pipelineDesc.DepthTest.DepthEnable = true;
			    pipelineDesc.DepthTest.DepthWriteEnable = true;
			    pipelineDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState, storage.WireframePipelineState.get()};
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
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<DirectLightingPass>
{
	using RuntimeType = RenderPassRuntimeTraits<DirectLightingPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<DirectLightingPass>;

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
		        .PassName = DirectLightingPass::PassName,
		        .PackageDeclarationName = "DirectLightingShaderPackage",
		        .Package = DirectLightingPass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"DirectLighting_RootSignature",
		        .PipelineStateDebugName = L"DirectLighting_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<IndirectLightingPass>
{
	using RuntimeType = RenderPassRuntimeTraits<IndirectLightingPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<IndirectLightingPass>;

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
		        .PassName = IndirectLightingPass::PassName,
		        .PackageDeclarationName = "IndirectLightingShaderPackage",
		        .Package = IndirectLightingPass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"IndirectLighting_RootSignature",
		        .PipelineStateDebugName = L"IndirectLighting_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<LightingCompositePass>
{
	using RuntimeType = RenderPassRuntimeTraits<LightingCompositePass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<LightingCompositePass>;

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
		        .PassName = LightingCompositePass::PassName,
		        .PackageDeclarationName = "LightingCompositeShaderPackage",
		        .Package = LightingCompositePass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"LightingComposite_RootSignature",
		        .PipelineStateDebugName = L"LightingComposite_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<SkyPass>
{
	using RuntimeType = RenderPassRuntimeTraits<SkyPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<SkyPass>;

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
		        .PassName = SkyPass::PassName,
		        .PackageDeclarationName = "SkyShaderPackage",
		        .Package = SkyPass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"Sky_RootSignature",
		        .PipelineStateDebugName = L"Sky_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};

template <> struct RenderPassPipelineTraits<VisualizeBuffersPass>
{
	using RuntimeType = RenderPassRuntimeTraits<VisualizeBuffersPass>::RuntimeType;
	using StorageType = RenderPassRuntimeStorage<VisualizeBuffersPass>;

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
		        .PassName = VisualizeBuffersPass::PassName,
		        .PackageDeclarationName = "VisualizeBuffersShaderPackage",
		        .Package = VisualizeBuffersPass::DescribeShaderPackage(),
		        .PipelineKind = RenderPassShaderPipelineKind::Compute,
		        .BindingLayoutDebugName = L"VisualizeBuffers_RootSignature",
		        .PipelineStateDebugName = L"VisualizeBuffers_PipelineState"},
		    storage,
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	static RuntimeType MakeRuntime(const StorageType& storage) noexcept
	{
		return RuntimeType{*storage.BindingLayout, *storage.PipelineState};
	}
};
