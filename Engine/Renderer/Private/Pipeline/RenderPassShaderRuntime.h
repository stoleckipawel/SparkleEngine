#pragma once

#include "Core/Public/Strings/StringUtils.h"
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
	template <typename ConfigurePipeline> static void CreateGraphicsRuntime(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    std::string_view shaderName,
	    const ShaderPackageDefinition& package,
	    bool allowInputAssemblerInputLayout,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipeline configurePipeline)
	{
		ValidateExpectedStages(shaderName, package, true);
		storage.BindingLayoutDefinition = BuildBindingLayout(shaderName, package);
		storage.ShaderPackage = &LoadShaderPackage(
		    renderHardwareInterface,
		    shaderPackageCache,
		    shaderName,
		    package,
		    allowInputAssemblerInputLayout,
		    storage.BindingLayoutDefinition);
		ValidatePackageCapabilities(
		    renderHardwareInterface,
		    shaderName,
		    package,
		    allowInputAssemblerInputLayout,
		    storage.BindingLayoutDefinition,
		    *storage.ShaderPackage);

		std::wstring bindingLayoutDebugName = Strings::ToWide(shaderName);
		bindingLayoutDebugName += L".BindingLayout";
		const PipelineRuntimePackageRequest packageRequest = BuildPackageRequest(
		    shaderName,
		    package,
		    allowInputAssemblerInputLayout,
		    storage.BindingLayoutDefinition,
		    bindingLayoutDebugName.c_str());
		storage.BindingLayout =
		    PipelineRuntimeLibrary::CreateBindingLayout(renderHardwareInterface, packageRequest, *storage.ShaderPackage);

		GraphicsPipelineDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.VertexShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Vertex};
		if (HasAnyShaderStageMask(package.ExpectedStages, ShaderStageMask::Pixel))
		{
			pipelineDesc.PixelShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Pixel};
		}
		std::wstring pipelineDebugName = Strings::ToWide(shaderName);
		pipelineDebugName += L".Pipeline";
		pipelineDesc.DebugName = pipelineDebugName.c_str();
		configurePipeline(pipelineDesc);
		storage.Pipeline = PipelineRuntimeLibrary::CreateGraphicsPipeline(renderHardwareInterface, pipelineDesc);
		storage.WireframePipeline.reset();
		storage.TwoSidedPipeline.reset();
		if (allowInputAssemblerInputLayout && !pipelineDesc.RenderWireframe)
		{
			GraphicsPipelineDesc twoSidedPipelineDesc = pipelineDesc;
			twoSidedPipelineDesc.CullMode = ERhiCullMode::None;
			storage.TwoSidedPipeline = PipelineRuntimeLibrary::CreateGraphicsPipeline(renderHardwareInterface, twoSidedPipelineDesc);

			GraphicsPipelineDesc wireframePipelineDesc = pipelineDesc;
			wireframePipelineDesc.RenderWireframe = true;
			wireframePipelineDesc.CullMode = ERhiCullMode::None;
			storage.WireframePipeline = PipelineRuntimeLibrary::CreateGraphicsPipeline(renderHardwareInterface, wireframePipelineDesc);
		}
	}

	template <typename ConfigurePipeline> static void CreateComputeRuntime(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    std::string_view shaderName,
	    const ShaderPackageDefinition& package,
	    RenderPassShaderRuntimeStorage& storage,
	    ConfigurePipeline configurePipeline)
	{
		ValidateExpectedStages(shaderName, package, false);
		storage.BindingLayoutDefinition = BuildBindingLayout(shaderName, package);
		storage.ShaderPackage =
		    &LoadShaderPackage(renderHardwareInterface, shaderPackageCache, shaderName, package, false, storage.BindingLayoutDefinition);
		ValidatePackageCapabilities(
		    renderHardwareInterface,
		    shaderName,
		    package,
		    false,
		    storage.BindingLayoutDefinition,
		    *storage.ShaderPackage);

		std::wstring bindingLayoutDebugName = Strings::ToWide(shaderName);
		bindingLayoutDebugName += L".BindingLayout";
		const PipelineRuntimePackageRequest packageRequest =
		    BuildPackageRequest(shaderName, package, false, storage.BindingLayoutDefinition, bindingLayoutDebugName.c_str());
		storage.BindingLayout =
		    PipelineRuntimeLibrary::CreateBindingLayout(renderHardwareInterface, packageRequest, *storage.ShaderPackage);

		ComputePipelineDesc pipelineDesc{};
		pipelineDesc.BindingLayout = storage.BindingLayout.get();
		pipelineDesc.ComputeShader = RhiShaderStageDesc{storage.ShaderPackage, ShaderStage::Compute};
		std::wstring pipelineDebugName = Strings::ToWide(shaderName);
		pipelineDebugName += L".Pipeline";
		pipelineDesc.DebugName = pipelineDebugName.c_str();
		configurePipeline(pipelineDesc);
		storage.Pipeline = PipelineRuntimeLibrary::CreateComputePipeline(renderHardwareInterface, pipelineDesc);
	}

private:
	static void ValidateExpectedStages(std::string_view shaderName, const ShaderPackageDefinition& package, bool graphics)
	{
		const bool hasVertex = HasAnyShaderStageMask(package.ExpectedStages, ShaderStageMask::Vertex);
		const bool hasCompute = HasAnyShaderStageMask(package.ExpectedStages, ShaderStageMask::Compute);
		const bool valid = graphics ? (hasVertex && !hasCompute) : (hasCompute && !hasVertex);
		if (valid)
			return;

		throw Diagnostics::Error(
		    std::format(
		        "Shader '{}' declares conflicting expected stages '{}' for its {} package '{}'",
		        shaderName,
		        FormatShaderStageMask(package.ExpectedStages),
		        graphics ? "graphics" : "compute",
		        package.PackageId != nullptr ? package.PackageId : "<null>"));
	}

	static PassParameterLayout BuildBindingLayout(std::string_view shaderName, const ShaderPackageDefinition& package)
	{
		if (!package.IsValid())
		{
			throw Diagnostics::Error(std::format("Shader '{}' declares an invalid cooked shader package", shaderName));
		}

		PassParameterLayout bindingLayout = BuildRegisteredShaderPackageLayout(package.PackageId);

		return bindingLayout;
	}

	static PipelineRuntimePackageRequest BuildPackageRequest(
	    std::string_view shaderName,
	    const ShaderPackageDefinition& package,
	    bool allowInputAssemblerInputLayout,
	    const PassParameterLayout& bindingLayout,
	    const wchar_t* bindingLayoutDebugName)
	{
		PipelineRuntimePackageRequest request{};
		request.PassName = shaderName;
		request.Package = package;
		request.BindingLayout = &bindingLayout;
		request.AllowInputAssemblerInputLayout = allowInputAssemblerInputLayout;
		request.BindingLayoutDebugName = bindingLayoutDebugName;
		return request;
	}

	static const LoadedShaderPackage& LoadShaderPackage(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    std::string_view shaderName,
	    const ShaderPackageDefinition& package,
	    bool allowInputAssemblerInputLayout,
	    const PassParameterLayout& bindingLayout)
	{
		return PipelineRuntimeLibrary::LoadShaderPackage(
		    renderHardwareInterface,
		    shaderPackageCache,
		    BuildPackageRequest(shaderName, package, allowInputAssemblerInputLayout, bindingLayout));
	}

	static void ValidatePackageCapabilities(
	    RenderHardwareInterface& renderHardwareInterface,
	    std::string_view shaderName,
	    const ShaderPackageDefinition& package,
	    bool allowInputAssemblerInputLayout,
	    const PassParameterLayout& bindingLayout,
	    const LoadedShaderPackage& shaderPackage)
	{
		PipelineRuntimeLibrary::ValidatePackageCapabilities(
		    renderHardwareInterface,
		    BuildPackageRequest(shaderName, package, allowInputAssemblerInputLayout, bindingLayout),
		    shaderPackage);
	}
};
