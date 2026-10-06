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
	const RenderSceneGpuResources& scene = resources.ImportedScene.Scene;

	if constexpr (requires { parameters->SceneTlas; })
	{
		parameters->SceneTlas = builder.CreateAccelerationStructureBinding(resources.SceneTlas);
	}
	if constexpr (requires { parameters->SkyTexture; })
	{
		parameters->SkyTexture = builder.CreateSRV(resources.ImportedScene.Sky);
	}
	if constexpr (requires { parameters->DirectionalLights; })
	{
		parameters->DirectionalLights = builder.CreateSRV(scene.Lighting.DirectionalLights);
	}
	if constexpr (requires { parameters->PointLights; })
	{
		parameters->PointLights = builder.CreateSRV(scene.Lighting.PointLights);
	}
	if constexpr (requires { parameters->SpotLights; })
	{
		parameters->SpotLights = builder.CreateSRV(scene.Lighting.SpotLights);
	}
	if constexpr (requires { parameters->RectLights; })
	{
		parameters->RectLights = builder.CreateSRV(scene.Lighting.RectLights);
	}
	if constexpr (requires { parameters->RayTracingHitVertices; })
	{
		parameters->RayTracingHitVertices = builder.CreateSRV(scene.RayTracing.Vertices);
	}
	if constexpr (requires { parameters->SkinInfluences; })
	{
		parameters->SkinInfluences = builder.CreateSRV(scene.RayTracing.SkinInfluences);
	}
	if constexpr (requires { parameters->MorphTargetDeltas; })
	{
		parameters->MorphTargetDeltas = builder.CreateSRV(scene.RayTracing.MorphTargetDeltas);
	}
	if constexpr (requires { parameters->RayTracingHitIndices; })
	{
		parameters->RayTracingHitIndices = builder.CreateSRV(scene.RayTracing.Indices);
	}
	if constexpr (requires { parameters->RayTracingHitInstances; })
	{
		parameters->RayTracingHitInstances = builder.CreateSRV(scene.RayTracing.Instances);
	}
	if constexpr (requires { parameters->RayTracingHitMaterials; })
	{
		parameters->RayTracingHitMaterials = builder.CreateSRV(scene.RayTracing.Materials);
	}
	if constexpr (requires { parameters->MeshInstances; })
	{
		parameters->MeshInstances = builder.CreateSRV(scene.Geometry.MeshInstances);
	}
	if constexpr (requires { parameters->JointMatrices; })
	{
		parameters->JointMatrices = builder.CreateSRV(scene.Geometry.JointMatrices);
	}
	if constexpr (requires { parameters->PreviousJointMatrices; })
	{
		parameters->PreviousJointMatrices = builder.CreateSRV(scene.Geometry.PreviousJointMatrices);
	}
	if constexpr (requires { parameters->MorphWeights; })
	{
		parameters->MorphWeights = builder.CreateSRV(scene.Geometry.MorphWeights);
	}
	if constexpr (requires { parameters->PreviousMorphWeights; })
	{
		parameters->PreviousMorphWeights = builder.CreateSRV(scene.Geometry.PreviousMorphWeights);
	}
	if constexpr (requires { parameters->SamplerLinearClamp; })
	{
		parameters->SamplerLinearClamp = RhiSamplerDesc{
		    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		    .MipFilter = RhiSamplerMipFilter::Linear,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	}

	if constexpr (requires { parameters->Frame; })
	{
		builder.AddParameterSetup<FrameUniformData>(
		    parameters,
		    [](auto& parameters, const FrameUniformData& frame) { parameters->Frame = frame; });
	}
	if constexpr (requires { parameters->View; } || requires { parameters->ViewCamera; } || requires { parameters->ViewTemporal; })
	{
		builder.AddParameterSetup<RenderView>(
		    parameters,
		    [](auto& parameters, const RenderView& view)
		    {
			    if constexpr (requires { parameters->View; })
			    {
				    parameters->View = view.uniform;
			    }
			    if constexpr (requires { parameters->ViewCamera; })
			    {
				    parameters->ViewCamera = view.cameraUniform;
			    }
			    if constexpr (requires { parameters->ViewTemporal; })
			    {
				    parameters->ViewTemporal = view.temporalUniform;
			    }
		    });
	}
	if constexpr (
	    requires { parameters->Sky; } || requires { parameters->SceneLighting; } || requires { parameters->RayTracingHitConstants; }
	    || requires { parameters->MaterialTextureTable; })
	{
		builder.AddParameterSetup<PreparedRenderScene>(
		    parameters,
		    [](auto& parameters, const PreparedRenderScene& preparedScene)
		    {
			    if constexpr (requires { parameters->Sky; })
			    {
				    parameters->Sky = MakeSkyUniformData(preparedScene.sky);
			    }
			    if constexpr (requires { parameters->SceneLighting; })
			    {
				    parameters->SceneLighting = preparedScene.gpuBindings->Lighting.Uniform;
			    }
			    if constexpr (requires { parameters->RayTracingHitConstants; })
			    {
				    parameters->RayTracingHitConstants = RayTracingHitUniformData{
				        .RayTracingHitInstanceCount = preparedScene.gpuBindings->RayTracing.InstanceCount,
				        .RayTracingHitMaterialCount = preparedScene.gpuBindings->RayTracing.MaterialCount};
			    }
			    if constexpr (requires { parameters->MaterialTextureTable; })
			    {
				    parameters->MaterialTextureTable = preparedScene.materialTextureTable.Binding;
			    }
		    });
	}
}
