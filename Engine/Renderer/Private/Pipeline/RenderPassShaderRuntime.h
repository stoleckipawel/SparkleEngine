#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "PipelineRuntime/PipelineRuntimeLibrary.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/ShaderParameterLayoutBuilder.h"

#include <array>
#include <format>
#include <memory>
#include <string>
#include <vector>

struct RenderPassShaderRuntimeStorage
{
	PassParameterLayout ParameterLayout;
	std::vector<ResolvedShader> Shaders;
	std::unique_ptr<RenderBindingLayout> BindingLayout;
};

class RenderPassShaderRuntime final
{
public:
	template <typename TVertexShader, typename TPixelShader> static void CreateGraphicsRuntime(
	    RenderHardwareInterface& renderHardwareInterface,
	    const ShaderRegistrationDesc& vertexRegistration,
	    ShaderRef<TVertexShader> vertexShader,
	    const ShaderRegistrationDesc& pixelRegistration,
	    ShaderRef<TPixelShader> pixelShader,
	    RenderPassShaderRuntimeStorage& storage)
	{
		if (!vertexShader || !pixelShader)
		{
			throw Diagnostics::Error("Graphics shader types did not resolve through the active map.");
		}
		const ShaderRegistrationDesc* registrations[] = {&vertexRegistration, &pixelRegistration};
		storage.ParameterLayout = BuildShaderPipelineParameterLayout(registrations);
		storage.Shaders = {vertexShader.GetResolvedShader(), pixelShader.GetResolvedShader()};
		ValidateResolvedShader(
		    renderHardwareInterface,
		    vertexRegistration,
		    storage.Shaders[0],
		    BuildShaderParameterLayout(vertexRegistration));
		ValidateResolvedShader(
		    renderHardwareInterface,
		    pixelRegistration,
		    storage.Shaders[1],
		    BuildShaderParameterLayout(pixelRegistration));
		std::wstring debugName = Strings::ToWide(vertexRegistration.ShaderName);
		storage.BindingLayout = PipelineRuntimeLibrary::CreateBindingLayout(
		    renderHardwareInterface,
		    storage.ParameterLayout,
		    storage.Shaders,
		    true,
		    debugName.c_str());
	}

	template <typename TShader, typename ConfigurePipeline> static void CreateComputeRuntime(
	    RenderHardwareInterface& renderHardwareInterface,
	    const ShaderRegistrationDesc& registration,
	    ShaderRef<TShader> shader,
	    RenderPassShaderRuntimeStorage& storage,
	    std::unique_ptr<RenderPipeline>& pipelineStorage,
	    ConfigurePipeline configurePipeline)
	{
		if (!shader)
		{
			throw Diagnostics::Error(std::format("Shader '{}' did not resolve through the active map.", registration.ShaderName));
		}
		storage.ParameterLayout = BuildShaderParameterLayout(registration);
		storage.Shaders = {shader.GetResolvedShader()};
		ValidateResolvedShader(renderHardwareInterface, registration, storage.Shaders.front(), storage.ParameterLayout);
		std::wstring debugName = Strings::ToWide(registration.ShaderName);
		storage.BindingLayout = PipelineRuntimeLibrary::CreateBindingLayout(
		    renderHardwareInterface,
		    storage.ParameterLayout,
		    storage.Shaders,
		    false,
		    debugName.c_str());
		ComputePipelineDesc pipeline;
		pipeline.BindingLayout = storage.BindingLayout.get();
		pipeline.ComputeShader = RhiShaderStageDesc{&storage.Shaders.front()};
		pipeline.DebugName = debugName.c_str();
		configurePipeline(pipeline);
		pipelineStorage = PipelineRuntimeLibrary::CreateComputePipeline(renderHardwareInterface, pipeline);
	}

private:
	static void ValidateResolvedShader(
	    RenderHardwareInterface& renderHardwareInterface,
	    const ShaderRegistrationDesc& registration,
	    const ResolvedShader& shader,
	    const PassParameterLayout& parameterLayout)
	{
		PipelineRuntimeLibrary::ValidateShaderCapabilities(renderHardwareInterface, registration.ShaderName, shader);
		if (shader.Entry->Stage != registration.Stage || shader.Entry->Features != registration.Features
		    || shader.Entry->ParameterSignature != BuildShaderParameterSignature(parameterLayout))
		{
			throw Diagnostics::Error(
			    std::format("Shader '{}' map metadata does not match its registered contract.", registration.ShaderName));
		}
	}
};
