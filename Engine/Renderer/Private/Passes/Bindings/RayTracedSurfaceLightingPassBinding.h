#pragma once

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/EnvironmentMapPassBinding.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracedSurfaceLightingPassParameters.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Bindings/RayTracingScenePassBinding.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

class RayTracedSurfaceLightingPassBinding final
{
  public:
	template <typename TParameters, typename TFields = RayTracedSurfaceLightingPassParameters>
	static void Describe(ShaderParameterStructBuilder<TParameters>& builder)
	{
		builder.AccelerationStructure("SceneTlas", PassMember<TParameters>(&TFields::SceneTlas), ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", PassMember<TParameters>(&TFields::PerFrame), ShaderStageVisibility::Compute);
		builder.Uniform("PerView", PassMember<TParameters>(&TFields::PerView), ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", PassMember<TParameters>(&TFields::ViewLighting), ShaderStageVisibility::Compute);
		builder.Uniform("RayTracedShadows", PassMember<TParameters>(&TFields::RayTracedShadows), ShaderStageVisibility::Compute);
#define DESCRIBE_RAY_TRACED_SURFACE_TEXTURE(Name) \
	builder.ReadTexture(#Name, PassMember<TParameters>(&TFields::Name), ShaderStageVisibility::Compute)
		DESCRIBE_RAY_TRACED_SURFACE_TEXTURE(GBufferBaseColor);
		DESCRIBE_RAY_TRACED_SURFACE_TEXTURE(GBufferNormal);
		DESCRIBE_RAY_TRACED_SURFACE_TEXTURE(GBufferMaterial);
		DESCRIBE_RAY_TRACED_SURFACE_TEXTURE(SceneDepth);
		DESCRIBE_RAY_TRACED_SURFACE_TEXTURE(SkyTexture);
#undef DESCRIBE_RAY_TRACED_SURFACE_TEXTURE
		builder.Sampler("SamplerLinearClamp", PassMember<TParameters>(&TFields::SamplerLinearClamp), ShaderStageVisibility::Compute);
#define DESCRIBE_RAY_TRACED_SURFACE_BUFFER(Name) \
	builder.ReadBuffer(#Name, PassMember<TParameters>(&TFields::Name), ShaderStageVisibility::Compute)
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(RayTracingHitVertices);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(RayTracingHitIndices);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(RayTracingHitInstances);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(RayTracingHitMaterials);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(MeshInstances);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(SkinInfluences);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(JointMatrices);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(DirectionalLights);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(PointLights);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(SpotLights);
		DESCRIBE_RAY_TRACED_SURFACE_BUFFER(RectLights);
#undef DESCRIBE_RAY_TRACED_SURFACE_BUFFER
		builder.ReadTexture(
		    "MaterialTextureTable",
		    PassMember<TParameters>(&TFields::MaterialTextureTable),
		    ShaderStageVisibility::Compute);
		builder.Sampler(
		    "MaterialTextureSampler",
		    PassMember<TParameters>(&TFields::MaterialTextureSampler),
		    ShaderStageVisibility::Compute);
	}

	template <typename TParameters>
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& scene,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    TypedPassParameterInstance<TParameters>& parameters)
	{
		(void) RayTracingScenePassBinding::BindSceneTlas(builder, sceneTlas, RayTracingSceneTlasShaderAccessMode::Descriptor, parameters);
		parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters->SceneDepth = builder.CreateSRV(scene.SceneDepth);
	}

	template <typename TParameters> bool Prepare(PassExecutionContext& context, TypedPassParameterInstance<TParameters>& parameters) const
	{
		const RayTracingPassCapabilities capabilities =
		    RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
		if (!capabilities.InlineRayQueryAvailable || !capabilities.HitDataAvailable || !capabilities.MaterialTextureTableAvailable ||
		    !RayTracingScenePassBinding::CanUseSceneTlas(capabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
		{
			return false;
		}

		const RhiDescriptorTableBinding environment = m_environmentMapBinding.GetTextureBinding(context.RuntimeServices);
		if (!environment || !MaterialTextureTablePassBinding::Bind(parameters, context.Frame))
		{
			return false;
		}

		parameters->PerFrame = context.RuntimeServices.PerFrame;
		parameters->PerView = context.Frame.mainView.perViewData;
		LightingPassBinding::SetParameters(parameters, context.Frame);
		RayTracingHitDataPassBinding::SetParameters(parameters, context.Frame);
		parameters->SkyTexture = environment;
		parameters->SamplerLinearClamp = RhiSamplerDesc{
		    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		    .MipFilter = RhiSamplerMipFilter::Linear,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
		parameters->MaterialTextureSampler = RhiSamplerDesc{
		    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		    .MipFilter = RhiSamplerMipFilter::Linear,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
		    .MaxAnisotropy = RhiSamplerAnisotropy::X1};
		parameters->RayTracedShadows = RayTracedShadowPassData::Build(
		    context.RuntimeServices.RayTracing,
		    context.Frame.rayTracingScene.HasTraceableInstances(),
		    capabilities.TriangleMaterialDataAvailable,
		    context.Frame.rayTracingHitData.GetInstanceCount(),
		    context.Frame.rayTracingHitData.GetMaterialCount());
		return true;
	}

  private:
	template <typename TParameters, typename TFields, typename TField>
	static constexpr TField TParameters::* PassMember(TField TFields::* member) noexcept
	{
		return static_cast<TField TParameters::*>(member);
	}

	mutable EnvironmentMapPassBinding m_environmentMapBinding;
};
