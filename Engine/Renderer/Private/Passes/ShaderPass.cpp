#include "../PCH.h"
#include "Passes/ShaderPass.h"

#include "Renderer/Public/FrameGraph/PassBuilder.h"
#include "GPU/CommandContext.h"
#include "Passes/ShaderSourceDefinition.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include "Core/Public/Diagnostics/Log.h"
#include "Pipeline/PassBinder.h"

#include <cassert>
#include <string>

namespace
{
	const char* GetShaderPassName(const char* passName) noexcept
	{
		return passName != nullptr && passName[0] != '\0' ? passName : "<unnamed>";
	}

	bool ReportShaderPassLayoutError(const char* passName, const std::string& message) noexcept
	{
		std::string logMessage = "Shader pass layout validation failed for pass '";
		logMessage += GetShaderPassName(passName);
		logMessage += "': ";
		logMessage += message;
		LOG_ERROR(logMessage);
		assert(false);
		return false;
	}
}  // namespace

void DeclareShaderPassParameterUsages(PassBuilder& builder, const PassParameterSet& parameterSet) noexcept
{
	builder.DeclareParameterUsages(parameterSet);
}

void DispatchComputeShaderPass(CommandContext& cmd, const ComputeDispatchDesc& dispatch) noexcept
{
	assert(dispatch.GroupCountX > 0);
	assert(dispatch.GroupCountY > 0);
	assert(dispatch.GroupCountZ > 0);
	cmd.Dispatch(dispatch.GroupCountX, dispatch.GroupCountY, dispatch.GroupCountZ);
}

bool ValidateShaderSourceDefinition(
    const ShaderSourceDefinition& sourceDefinition,
    ShaderStage expectedStage,
    const char* passName,
    const char* shaderLabel) noexcept
{
	if (!sourceDefinition.IsValid())
	{
		std::string message = "Shader pass '";
		message += GetShaderPassName(passName);
		message += "' is missing a valid shader declaration for '";
		message += shaderLabel != nullptr && shaderLabel[0] != '\0' ? shaderLabel : "<unnamed shader>";
		message += "'.";
		LOG_ERROR(message);
		assert(false);
		return false;
	}

	if (sourceDefinition.GetStage() != expectedStage)
	{
		std::string message = "Shader pass '";
		message += GetShaderPassName(passName);
		message += "' declared shader '";
		message += shaderLabel != nullptr && shaderLabel[0] != '\0' ? shaderLabel : sourceDefinition.GetEntryPoint().c_str();
		message += "' with stage ";
		message += std::to_string(static_cast<int>(sourceDefinition.GetStage()));
		message += " but expected stage ";
		message += std::to_string(static_cast<int>(expectedStage));
		message += ".";
		LOG_ERROR(message);
		assert(false);
		return false;
	}

	return true;
}

bool ValidateShaderPassLayout(const PassParameterLayout& layout, ShaderPassKind passKind, const char* passName) noexcept
{
	for (const PassParameterDesc& parameter : layout.GetParameters())
	{
		if (parameter.Name.empty())
		{
			return ReportShaderPassLayoutError(passName, "encountered a parameter with no name.");
		}

		if (parameter.Visibility == ShaderStageVisibility::None)
		{
			return ReportShaderPassLayoutError(passName, "parameter '" + parameter.Name + "' has no shader stage visibility.");
		}

		if (passKind == ShaderPassKind::Compute)
		{
			if (HasAnyShaderStageVisibility(parameter.Visibility, ShaderStageVisibility::Vertex | ShaderStageVisibility::Pixel))
			{
				return ReportShaderPassLayoutError(
				    passName,
				    "parameter '" + parameter.Name + "' exposes graphics-only stage visibility on a compute pass.");
			}

			if (parameter.Kind == ShaderParameterSemanticKind::RenderTarget || parameter.Kind == ShaderParameterSemanticKind::DepthTarget)
			{
				return ReportShaderPassLayoutError(
				    passName,
				    "parameter '" + parameter.Name + "' uses a raster-only resource kind on a compute pass.");
			}
		}
		else
		{
			if (HasAnyShaderStageVisibility(parameter.Visibility, ShaderStageVisibility::Compute))
			{
				return ReportShaderPassLayoutError(
				    passName,
				    "parameter '" + parameter.Name + "' exposes compute stage visibility on a raster pass.");
			}
		}
	}

	return true;
}

void BindComputeShaderPass(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
	RenderHardwareInterface* renderHardwareInterface,
	const RenderBindingLayout& bindingLayout,
	const RenderPipelineState& pipelineState,
    const PassParameterSet& parameterSet) noexcept
{
	if (renderHardwareInterface != nullptr)
	{
		renderHardwareInterface->SetShaderVisibleDescriptorHeaps(cmd.GetRenderCommandList());
	}

	cmd.SetPipelineState(pipelineState);
	PassBinder::BindCompute(cmd, frameGraph, bindingLayout, parameterSet);
}

void BindRasterShaderPass(
    CommandContext& cmd,
    const FrameGraph& frameGraph,
	RenderHardwareInterface* renderHardwareInterface,
	const RenderBindingLayout& bindingLayout,
	const RenderPipelineState& pipelineState,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames,
    std::uint32_t bindingNameCount,
    const PassBindingOverrides* overrides) noexcept
{
	if (renderHardwareInterface != nullptr)
	{
		renderHardwareInterface->SetShaderVisibleDescriptorHeaps(cmd.GetRenderCommandList());
	}

	cmd.SetPipelineState(pipelineState);
	PassBinder::BindGraphics(
	    cmd,
	    frameGraph,
	    bindingLayout,
	    parameterSet,
	    bindingNames != nullptr ? std::span<const char* const>(bindingNames, bindingNameCount) : std::span<const char* const>{},
	    overrides);
}

void ReportInvalidShaderPassParameterSet(const char* passName, const PassParameterSet& parameterSet) noexcept
{
	std::string message = "Shader pass parameter validation failed";
	if (passName != nullptr && passName[0] != '\0')
	{
		message += " for pass '";
		message += passName;
		message += "'";
	}

	if (!parameterSet.HasLayout())
	{
		message += ": missing parameter layout.";
		LOG_ERROR(message);
		assert(false);
		return;
	}

	const std::vector<std::string> missingBindings = parameterSet.GetMissingBindings();
	if (missingBindings.empty())
	{
		message += ": parameter bindings are incomplete.";
		LOG_ERROR(message);
		assert(false);
		return;
	}

	message += ": missing bindings ";
	for (std::size_t index = 0; index < missingBindings.size(); ++index)
	{
		if (index > 0)
		{
			message += ", ";
		}

		message += missingBindings[index];
	}
	message += ".";
	LOG_ERROR(message);
	assert(false);
}