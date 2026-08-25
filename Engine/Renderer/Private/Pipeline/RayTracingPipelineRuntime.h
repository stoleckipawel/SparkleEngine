#pragma once

#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RayTracingPipelineComposition.h"
#include "RHI/Public/RayTracing/RhiRayTracingPipelineDesc.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <memory>
#include <vector>

class CookedShaderLibrary;
class GlobalShaderMap;
class RenderHardwareInterface;

class RayTracingPipelineRuntime final
{
public:
	static std::unique_ptr<RayTracingPipelineRuntime> Create(
	    RenderHardwareInterface& renderHardwareInterface,
	    const GlobalShaderMap& map,
	    const CookedShaderLibrary& library,
	    ShaderTarget target,
	    std::uint64_t generation,
	    const RayTracingPipelineComposition& composition);

	RenderBindingLayout& GetBindingLayout() const noexcept { return *m_bindingLayout; }
	RayTracingPipeline& GetPipeline() const noexcept { return *m_pipeline; }
	RayTracingShaderTable& GetShaderTable() const noexcept { return *m_shaderTable; }
	std::uint64_t GetGeneration() const noexcept { return m_generation; }

private:
	std::uint64_t m_generation = 0;
	PassParameterLayout m_parameterLayout;
	std::vector<ResolvedShader> m_shaders;
	std::unique_ptr<RenderBindingLayout> m_bindingLayout;
	std::unique_ptr<RayTracingPipeline> m_pipeline;
	std::unique_ptr<RayTracingShaderTable> m_shaderTable;
};
