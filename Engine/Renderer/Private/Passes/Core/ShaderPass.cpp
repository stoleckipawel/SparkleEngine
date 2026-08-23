#include "../../PCH.h"
#include "Passes/Core/ShaderPass.h"

#include "FrameGraph/Builder/PassResourceBuilder.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "Commands/RenderCommandContext.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Pipeline/PassBinder.h"

#include <cassert>
#include <string>

static const auto g_shaderPassLogger = Logging::GetOrCreateLogger("Renderer");

bool DeclareShaderPassParameterUsages(PassResourceBuilder& builder, const PassParameterSet& parameterSet, const char* passName) noexcept
{
	return builder.DeclareParameterUsages(parameterSet, passName != nullptr ? passName : "");
}

void DispatchComputeShaderPass(RenderCommandContext& commandContext, const ComputeDispatchDesc& dispatch) noexcept
{
	assert(dispatch.GroupCountX > 0);
	assert(dispatch.GroupCountY > 0);
	assert(dispatch.GroupCountZ > 0);
	commandContext.Dispatch(dispatch.GroupCountX, dispatch.GroupCountY, dispatch.GroupCountZ);
}

static bool HasBindingOverride(const PassBindingOverrides* overrides, const char* bindingName) noexcept
{
	if (overrides == nullptr || bindingName == nullptr)
	{
		return false;
	}

	for (const PassBindingOverride& bindingOverride : overrides->GetOverrides())
	{
		if (bindingOverride.Name == bindingName)
		{
			return true;
		}
	}

	return false;
}

bool ValidateShaderParameterSetup(const PassParameterSet& parameterSet, const char* passName) noexcept
{
	if (!parameterSet.HasLayout())
	{
		ReportInvalidShaderPassParameterSet(passName, parameterSet);
		return false;
	}

	const PassParameterLayout* layout = parameterSet.GetLayout();
	if (layout == nullptr)
	{
		ReportInvalidShaderPassParameterSet(passName, parameterSet);
		return false;
	}

	const std::vector<PassParameterDesc>& parameters = layout->GetParameters();
	for (std::uint32_t index = 0; index < static_cast<std::uint32_t>(parameters.size()); ++index)
	{
		if (!parameterSet.UsesGraphResource(index))
		{
			continue;
		}

		const PassParameterBinding* binding = parameterSet.GetBinding(index);
		if (binding == nullptr || !binding->IsBound())
		{
			ReportInvalidShaderPassParameterSet(passName, parameterSet);
			return false;
		}
	}

	return true;
}

bool ValidateShaderParameters(
    const PassParameterSet& parameterSet,
    const char* passName,
    const char* const* bindingNames,
    std::uint32_t bindingNameCount,
    const PassBindingOverrides* overrides) noexcept
{
	if (!parameterSet.HasLayout())
	{
		ReportInvalidShaderPassParameterSet(passName, parameterSet);
		return false;
	}

	if (bindingNames != nullptr && bindingNameCount > 0)
	{
		for (std::uint32_t index = 0; index < bindingNameCount; ++index)
		{
			const PassParameterBinding* binding = parameterSet.FindBinding(bindingNames[index]);
			if (binding != nullptr && !binding->IsBound() && !HasBindingOverride(overrides, bindingNames[index]))
			{
				ReportInvalidShaderPassParameterSet(passName, parameterSet);
				return false;
			}
		}
		return true;
	}

	if (!parameterSet.HasAllRequiredBindings())
	{
		ReportInvalidShaderPassParameterSet(passName, parameterSet);
		return false;
	}

	return true;
}

void BindComputeShaderPass(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames,
    std::uint32_t bindingNameCount,
    const PassBindingOverrides* overrides,
    bool bindLayout) noexcept
{
	if (bindLayout)
	{
		resources.BindGlobalDescriptorState(commandContext);
	}

	commandContext.SetPipeline(pipeline);
	PassBinder::BindCompute(
	    commandContext,
	    resources,
	    bindingLayout,
	    parameterSet,
	    bindingNames != nullptr ? std::span<const char* const>(bindingNames, bindingNameCount) : std::span<const char* const>{},
	    overrides,
	    bindLayout);
}

void BindRasterShaderPass(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames,
    std::uint32_t bindingNameCount,
    const PassBindingOverrides* overrides,
    bool bindLayout) noexcept
{
	if (bindLayout)
	{
		resources.BindGlobalDescriptorState(commandContext);
	}

	commandContext.SetPipeline(pipeline);
	PassBinder::BindGraphics(
	    commandContext,
	    resources,
	    bindingLayout,
	    parameterSet,
	    bindingNames != nullptr ? std::span<const char* const>(bindingNames, bindingNameCount) : std::span<const char* const>{},
	    overrides,
	    bindLayout);
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
		SPDLOG_LOGGER_ERROR(g_shaderPassLogger, "{}", message);
		assert(false);
		return;
	}

	const std::vector<std::string> missingBindings = parameterSet.GetMissingBindings();
	if (missingBindings.empty())
	{
		message += ": parameter bindings are incomplete.";
		SPDLOG_LOGGER_ERROR(g_shaderPassLogger, "{}", message);
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
	SPDLOG_LOGGER_ERROR(g_shaderPassLogger, "{}", message);
	assert(false);
}
