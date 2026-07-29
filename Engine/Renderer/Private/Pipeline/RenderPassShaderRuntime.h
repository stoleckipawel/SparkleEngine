#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "PipelineRuntime/PipelineRuntimeLibrary.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/CookedShaderPackageCache.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutBuilder.h"

#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

class RenderHardwareInterface;

enum class RenderPassShaderPipelineKind
{
	Graphics,
	Compute,
};

struct RenderPassShaderRuntimeDesc final
{
	std::string_view PassName;
	ShaderPackageDefinition Package;
	RenderPassShaderPipelineKind PipelineKind = RenderPassShaderPipelineKind::Graphics;
	bool AllowInputAssemblerInputLayout = false;
	const wchar_t* BindingLayoutDebugName = L"RenderPass_BindingLayout";
	const wchar_t* PipelineDebugName = L"RenderPass_Pipeline";
};

struct RenderPassShaderRuntimeStorage
{
	PassParameterLayout BindingLayoutDefinition;
	std::unique_ptr<RenderBindingLayout> BindingLayout;
	std::unique_ptr<RenderPipeline> Pipeline;
	std::unique_ptr<RenderPipeline> WireframePipeline;
	std::unique_ptr<RenderPipeline> TwoSidedPipeline;
	const LoadedShaderPackage* ShaderPackage = nullptr;
};

class RenderPassShaderRuntime final
{
  public:
	template <typename ConfigurePipeline>
	static void CreateGraphicsRuntime(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipeline configurePipeline)
	{
		ValidatePipelineKind(desc, RenderPassShaderPipelineKind::Graphics);
		ValidateExpectedStages(desc);
		storage.BindingLayoutDefinition = BuildBindingLayout(desc);
		storage.ShaderPackage =
		    &LoadShaderPackage(renderHardwareInterface, shaderPackageCache, desc, storage.BindingLayoutDefinition);
		ValidatePackageCapabilities(renderHardwareInterface, desc, storage.BindingLayoutDefinition, *storage.ShaderPackage);

		const PipelineRuntimePackageRequest packageRequest = BuildPackageRequest(desc, storage.BindingLayoutDefinition);
		storage.BindingLayout = PipelineRuntimeLibrary::CreateBindingLayout(renderHardwareInterface, packageRequest, *storage.ShaderPackage);

		GraphicsPipelineDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Vertex};
		if (HasAnyShaderStageMask(desc.Package.ExpectedStages, ShaderStageMask::Pixel))
		{
			pipelineDesc.PixelShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Pixel};
		}
		pipelineDesc.DebugName = desc.PipelineDebugName;
		configurePipeline(pipelineDesc);
		storage.Pipeline =
		    PipelineRuntimeLibrary::CreateGraphicsPipeline(renderHardwareInterface, pipelineDesc);
		storage.WireframePipeline.reset();
		storage.TwoSidedPipeline.reset();
		if (desc.AllowInputAssemblerInputLayout && !pipelineDesc.RenderWireframe)
		{
			GraphicsPipelineDesc twoSidedPipelineDesc = pipelineDesc;
			twoSidedPipelineDesc.CullMode = ERhiCullMode::None;
			storage.TwoSidedPipeline = PipelineRuntimeLibrary::CreateGraphicsPipeline(
			    renderHardwareInterface,
			    twoSidedPipelineDesc);

			GraphicsPipelineDesc wireframePipelineDesc = pipelineDesc;
			wireframePipelineDesc.RenderWireframe = true;
			wireframePipelineDesc.CullMode = ERhiCullMode::None;
			storage.WireframePipeline = PipelineRuntimeLibrary::CreateGraphicsPipeline(
			    renderHardwareInterface,
			    wireframePipelineDesc);
		}
	}

	template <typename ConfigurePipeline>
	static void CreateComputeRuntime(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipeline configurePipeline)
	{
		ValidatePipelineKind(desc, RenderPassShaderPipelineKind::Compute);
		ValidateExpectedStages(desc);
		storage.BindingLayoutDefinition = BuildBindingLayout(desc);
		storage.ShaderPackage =
		    &LoadShaderPackage(renderHardwareInterface, shaderPackageCache, desc, storage.BindingLayoutDefinition);
		ValidatePackageCapabilities(renderHardwareInterface, desc, storage.BindingLayoutDefinition, *storage.ShaderPackage);

		const PipelineRuntimePackageRequest packageRequest = BuildPackageRequest(desc, storage.BindingLayoutDefinition);
		storage.BindingLayout = PipelineRuntimeLibrary::CreateBindingLayout(renderHardwareInterface, packageRequest, *storage.ShaderPackage);

		ComputePipelineDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.ComputeShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Compute};
		pipelineDesc.DebugName = desc.PipelineDebugName;
		configurePipeline(pipelineDesc);
		storage.Pipeline =
		    PipelineRuntimeLibrary::CreateComputePipeline(renderHardwareInterface, pipelineDesc);
	}

  private:
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

	static void ValidatePipelineKind(
	    const RenderPassShaderRuntimeDesc& desc,
	    RenderPassShaderPipelineKind expectedKind)
	{
		if (desc.PipelineKind == expectedKind)
			return;

		throw Diagnostics::Error(std::format(
		    "Render pass '{}' requested a {} shader runtime through the {} facade path for package '{}'",
		    desc.PassName,
		    FormatPipelineKind(desc.PipelineKind),
		    FormatPipelineKind(expectedKind),
		    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>"));
	}

	static void ValidateExpectedStages(const RenderPassShaderRuntimeDesc& desc)
	{
		const bool hasVertex = HasAnyShaderStageMask(desc.Package.ExpectedStages, ShaderStageMask::Vertex);
		const bool hasCompute = HasAnyShaderStageMask(desc.Package.ExpectedStages, ShaderStageMask::Compute);
		const bool isGraphics = desc.PipelineKind == RenderPassShaderPipelineKind::Graphics;
		const bool valid = isGraphics ? (hasVertex && !hasCompute) : (hasCompute && !hasVertex);
		if (valid)
			return;

		throw Diagnostics::Error(std::format(
		    "Render pass '{}' declares conflicting expected stages '{}' for {} shader package '{}'",
		    desc.PassName,
		    FormatShaderStageMask(desc.Package.ExpectedStages),
		    FormatPipelineKind(desc.PipelineKind),
		    desc.Package.PackageId != nullptr ? desc.Package.PackageId : "<null>"));
	}

	static PassParameterLayout BuildBindingLayout(const RenderPassShaderRuntimeDesc& desc)
	{
		if (!desc.Package.IsValid())
		{
			throw Diagnostics::Error(
			    std::format("Render pass '{}' declares an invalid cooked shader package", desc.PassName));
		}

		PassParameterLayout bindingLayout = BuildRegisteredShaderPackageLayout(desc.Package.PackageId);

		return bindingLayout;
	}

	static PipelineRuntimePackageRequest BuildPackageRequest(const RenderPassShaderRuntimeDesc& desc, const PassParameterLayout& bindingLayout)
	{
		PipelineRuntimePackageRequest request{};
		request.PassName = desc.PassName;
		request.Package = desc.Package;
		request.BindingLayout = &bindingLayout;
		request.AllowInputAssemblerInputLayout = desc.AllowInputAssemblerInputLayout;
		request.BindingLayoutDebugName = desc.BindingLayoutDebugName;
		return request;
	}

	static const LoadedShaderPackage& LoadShaderPackage(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassShaderRuntimeDesc& desc,
	    const PassParameterLayout& bindingLayout)
	{
		return PipelineRuntimeLibrary::LoadShaderPackage(
		    renderHardwareInterface,
		    shaderPackageCache,
		    BuildPackageRequest(desc, bindingLayout));
	}

	static void ValidatePackageCapabilities(
	    RenderHardwareInterface& renderHardwareInterface,
	    const RenderPassShaderRuntimeDesc& desc,
	    const PassParameterLayout& bindingLayout,
	    const LoadedShaderPackage& shaderPackage)
	{
		PipelineRuntimeLibrary::ValidatePackageCapabilities(
		    renderHardwareInterface,
		    BuildPackageRequest(desc, bindingLayout),
		    shaderPackage);
	}

};
