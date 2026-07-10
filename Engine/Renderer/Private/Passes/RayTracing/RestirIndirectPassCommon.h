#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Passes/Bindings/RayTracedSurfaceLightingPassBinding.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingSettings.h"

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;

struct RestirIndirectScenePassParameters : RayTracedSurfaceLightingPassParameters
{
	ShaderUniform<RestirIndirectLightingUniformData> RestirIndirectConstants;
};

namespace RestirIndirectPassCommon
{
	template <typename TParameters> void DescribeSceneParameters(ShaderParameterStructBuilder<TParameters>& builder)
	{
		RayTracedSurfaceLightingPassBinding::Describe<TParameters>(builder);
		const auto restirConstants = static_cast<ShaderUniform<RestirIndirectLightingUniformData> TParameters::*>(
		    &RestirIndirectScenePassParameters::RestirIndirectConstants);
		builder.Uniform("RestirIndirectConstants", restirConstants, ShaderStageVisibility::Compute);
	}

	template <typename TParameters>
	void BindSceneResources(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& scene,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    TypedPassParameterInstance<TParameters>& parameters)
	{
		RayTracedSurfaceLightingPassBinding::DeclareResources(builder, scene, gbuffer, sceneTlas, parameters);
	}
}

template <typename TParameters> class RestirIndirectPassBase
{
  public:
	using Parameters = TParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

  protected:
	explicit RestirIndirectPassBase(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}
	bool PrepareExecution(PassExecutionContext& context, ParameterInstance& parameters) const
	{
		if (!m_sceneBinding.Prepare(context, parameters))
		{
			return false;
		}

		const RestirIndirectLightingSettings settings = BuildRestirIndirectLightingSettings();
		parameters->RestirIndirectConstants = RestirIndirectLightingUniformData{
		    .BounceCount = settings.BounceCount,
		    .NormalBias = settings.NormalBias,
		    .MaxDistance = settings.MaxDistance};
		return true;
	}

	const ComputePassPipelineRuntime& m_runtime;
	mutable RayTracedSurfaceLightingPassBinding m_sceneBinding;
};
