#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/RayTracingHitUniformData.h"
#include "ShaderData/SkyUniformData.h"
#include "View/RenderView.h"

template <typename TParameterInstance>
void BindSceneShaderParameters(FrameGraphBuilder& builder, TParameterInstance& parameters, const RenderFrameGraphResources& resources)
{
	auto* fields = parameters.operator->();
	const RenderSceneGpuResources& scene = resources.ImportedScene.Scene;

	if constexpr (requires { fields->SceneTlas; })
	{
		fields->SceneTlas = builder.CreateAccelerationStructureBinding(resources.SceneTlas);
	}
	if constexpr (requires { fields->SkyTexture; })
	{
		fields->SkyTexture = builder.CreateSRV(resources.ImportedScene.Sky);
	}
	if constexpr (requires { fields->DirectionalLights; })
	{
		fields->DirectionalLights = builder.CreateSRV(scene.Lighting.DirectionalLights);
	}
	if constexpr (requires { fields->PointLights; })
	{
		fields->PointLights = builder.CreateSRV(scene.Lighting.PointLights);
	}
	if constexpr (requires { fields->SpotLights; })
	{
		fields->SpotLights = builder.CreateSRV(scene.Lighting.SpotLights);
	}
	if constexpr (requires { fields->RectLights; })
	{
		fields->RectLights = builder.CreateSRV(scene.Lighting.RectLights);
	}
	if constexpr (requires { fields->RayTracingHitVertices; })
	{
		fields->RayTracingHitVertices = builder.CreateSRV(scene.RayTracing.Vertices);
	}
	if constexpr (requires { fields->SkinInfluences; })
	{
		fields->SkinInfluences = builder.CreateSRV(scene.RayTracing.SkinInfluences);
	}
	if constexpr (requires { fields->MorphTargetDeltas; })
	{
		fields->MorphTargetDeltas = builder.CreateSRV(scene.RayTracing.MorphTargetDeltas);
	}
	if constexpr (requires { fields->RayTracingHitIndices; })
	{
		fields->RayTracingHitIndices = builder.CreateSRV(scene.RayTracing.Indices);
	}
	if constexpr (requires { fields->RayTracingHitInstances; })
	{
		fields->RayTracingHitInstances = builder.CreateSRV(scene.RayTracing.Instances);
	}
	if constexpr (requires { fields->RayTracingHitMaterials; })
	{
		fields->RayTracingHitMaterials = builder.CreateSRV(scene.RayTracing.Materials);
	}
	if constexpr (requires { fields->MeshInstances; })
	{
		fields->MeshInstances = builder.CreateSRV(scene.Geometry.MeshInstances);
	}
	if constexpr (requires { fields->JointMatrices; })
	{
		fields->JointMatrices = builder.CreateSRV(scene.Geometry.JointMatrices);
	}
	if constexpr (requires { fields->PreviousJointMatrices; })
	{
		fields->PreviousJointMatrices = builder.CreateSRV(scene.Geometry.PreviousJointMatrices);
	}
	if constexpr (requires { fields->MorphWeights; })
	{
		fields->MorphWeights = builder.CreateSRV(scene.Geometry.MorphWeights);
	}
	if constexpr (requires { fields->PreviousMorphWeights; })
	{
		fields->PreviousMorphWeights = builder.CreateSRV(scene.Geometry.PreviousMorphWeights);
	}
	if constexpr (requires { fields->SamplerLinearClamp; })
	{
		fields->SamplerLinearClamp = RhiSamplerDesc{
		    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		    .MipFilter = RhiSamplerMipFilter::Linear,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	}

	if constexpr (requires { fields->Frame; })
	{
		builder.AddParameterSetup<FrameUniformData>(parameters, [](auto& values, const FrameUniformData& frame) { values.Frame = frame; });
	}
	if constexpr (requires { fields->View; } || requires { fields->ViewCamera; } || requires { fields->ViewTemporal; })
	{
		builder.AddParameterSetup<RenderView>(
		    parameters,
		    [](auto& values, const RenderView& view)
		    {
			    if constexpr (requires { values.View; })
			    {
				    values.View = view.uniform;
			    }
			    if constexpr (requires { values.ViewCamera; })
			    {
				    values.ViewCamera = view.cameraUniform;
			    }
			    if constexpr (requires { values.ViewTemporal; })
			    {
				    values.ViewTemporal = view.temporalUniform;
			    }
		    });
	}
	if constexpr (
	    requires { fields->Sky; } || requires { fields->SceneLighting; } || requires { fields->RayTracingHitConstants; }
	    || requires { fields->MaterialTextureTable; })
	{
		builder.AddParameterSetup<PreparedRenderScene>(
		    parameters,
		    [](auto& values, const PreparedRenderScene& preparedScene)
		    {
			    if constexpr (requires { values.Sky; })
			    {
				    values.Sky = MakeSkyUniformData(preparedScene.sky);
			    }
			    if constexpr (requires { values.SceneLighting; })
			    {
				    values.SceneLighting = preparedScene.gpuBindings->Lighting.Uniform;
			    }
			    if constexpr (requires { values.RayTracingHitConstants; })
			    {
				    values.RayTracingHitConstants = RayTracingHitUniformData{
				        .RayTracingHitInstanceCount = preparedScene.gpuBindings->RayTracing.InstanceCount,
				        .RayTracingHitMaterialCount = preparedScene.gpuBindings->RayTracing.MaterialCount};
			    }
			    if constexpr (requires { values.MaterialTextureTable; })
			    {
				    values.MaterialTextureTable = preparedScene.materialTextureTable.Binding;
			    }
		    });
	}
}
