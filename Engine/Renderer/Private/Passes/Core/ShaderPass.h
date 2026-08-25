#pragma once

#include "Pipeline/PassBindingOverrides.h"
#include "ShaderParameters/PassParameterSet.h"

#include <cstdint>
#include <type_traits>

class RenderCommandContext;
class FrameGraphResourceCommands;
class PassResourceBuilder;
class RenderBindingLayout;
class RenderPipeline;
class RayTracingPipeline;

struct ComputeDispatchDesc
{
	std::uint32_t GroupCountX = 1;
	std::uint32_t GroupCountY = 1;
	std::uint32_t GroupCountZ = 1;
};

struct RayTracingDispatchDimensions final
{
	std::uint32_t Width = 1;
	std::uint32_t Height = 1;
	std::uint32_t Depth = 1;
};

bool DeclareShaderPassParameterUsages(
    PassResourceBuilder& builder,
    const PassParameterSet& parameterSet,
    const char* passName = nullptr) noexcept;
bool ValidateShaderParameterSetup(const PassParameterSet& parameterSet, const char* passName) noexcept;
bool ValidateShaderParameters(
    const PassParameterSet& parameterSet,
    const char* passName,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr) noexcept;
void DispatchComputeShaderPass(RenderCommandContext& commandContext, const ComputeDispatchDesc& dispatch) noexcept;
void BindComputeShaderPass(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr,
    bool bindLayout = true) noexcept;

void BindRasterShaderPass(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const PassParameterSet& parameterSet,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr,
    bool bindLayout = true) noexcept;
void BindRayTracingShaderPass(
    RenderCommandContext& commandContext,
    const FrameGraphResourceCommands& resources,
    const RenderBindingLayout& bindingLayout,
    const RayTracingPipeline& pipeline,
    const PassParameterSet& parameterSet) noexcept;

void ReportInvalidShaderPassParameterSet(const char* passName, const PassParameterSet& parameterSet) noexcept;

namespace ShaderPassDetail
{
	template <typename TParameterBindings> const PassParameterSet& GetPassParameterSet(const TParameterBindings& parameters) noexcept
	{
		if constexpr (std::is_same_v<std::remove_cvref_t<TParameterBindings>, PassParameterSet>)
		{
			return parameters;
		}
		else
		{
			static_assert(
			    requires(const TParameterBindings& value) { value.GetPassParameterSet(); },
			    "Shader parameters must expose GetPassParameterSet() or be PassParameterSet.");
			return parameters.GetPassParameterSet();
		}
	}
}

template <typename TParameterBindings>
bool SetupShaderParameters(PassResourceBuilder& builder, const TParameterBindings& parameters, const char* passName = nullptr) noexcept
{
	const PassParameterSet& parameterSet = ShaderPassDetail::GetPassParameterSet(parameters);
	if (!ValidateShaderParameterSetup(parameterSet, passName))
	{
		return false;
	}
	return DeclareShaderPassParameterUsages(builder, parameterSet, passName);
}

template <typename TParameterBindings> bool DispatchComputeShader(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const TParameterBindings& parameters,
    const ComputeDispatchDesc& dispatch,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr,
    const char* passName = nullptr,
    bool bindLayout = true) noexcept
{
	const PassParameterSet& parameterSet = ShaderPassDetail::GetPassParameterSet(parameters);
	if (!ValidateShaderParameters(parameterSet, passName, bindingNames, bindingNameCount, overrides))
	{
		return false;
	}

	BindComputeShaderPass(
	    commandContext,
	    resources,
	    bindingLayout,
	    pipeline,
	    parameterSet,
	    bindingNames,
	    bindingNameCount,
	    overrides,
	    bindLayout);
	DispatchComputeShaderPass(commandContext, dispatch);
	return true;
}

template <typename TParameterBindings> bool BindRasterShader(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const RenderBindingLayout& bindingLayout,
    const RenderPipeline& pipeline,
    const TParameterBindings& parameters,
    const char* const* bindingNames = nullptr,
    std::uint32_t bindingNameCount = 0,
    const PassBindingOverrides* overrides = nullptr,
    const char* passName = nullptr,
    bool bindLayout = true) noexcept
{
	const PassParameterSet& parameterSet = ShaderPassDetail::GetPassParameterSet(parameters);
	if (!ValidateShaderParameters(parameterSet, passName, bindingNames, bindingNameCount, overrides))
	{
		return false;
	}

	BindRasterShaderPass(
	    commandContext,
	    resources,
	    bindingLayout,
	    pipeline,
	    parameterSet,
	    bindingNames,
	    bindingNameCount,
	    overrides,
	    bindLayout);
	return true;
}
