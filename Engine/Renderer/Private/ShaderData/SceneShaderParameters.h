#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/RayTracingHitUniformData.h"
#include "ShaderData/SkyUniformData.h"
#include "View/RenderView.h"

template <typename TParameterInstance> void BindSceneShaderParameters(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    TParameterInstance& parameters,
    const RenderFrameGraphResources& resources)
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
		parameters->Frame = BuildFrameUniformData(frame.Identity.FrameId, frame.Time);
	}
	if constexpr (requires { parameters->View; } || requires { parameters->ViewCamera; } || requires { parameters->ViewTemporal; })
	{
		if constexpr (requires { parameters->View; })
		{
			parameters->View = frame.View.uniform;
		}
		if constexpr (requires { parameters->ViewCamera; })
		{
			parameters->ViewCamera = frame.View.cameraUniform;
		}
		if constexpr (requires { parameters->ViewTemporal; })
		{
			parameters->ViewTemporal = frame.View.temporalUniform;
		}
	}
	if constexpr (
	    requires { parameters->Sky; } || requires { parameters->SceneLighting; } || requires { parameters->RayTracingHitConstants; }
	    || requires { parameters->MaterialTextureTable; })
	{
		if constexpr (requires { parameters->Sky; })
		{
			parameters->Sky = MakeSkyUniformData(frame.PreparedScene.sky);
		}
		if constexpr (requires { parameters->SceneLighting; })
		{
			parameters->SceneLighting = frame.PreparedScene.gpuBindings->Lighting.Uniform;
		}
		if constexpr (requires { parameters->RayTracingHitConstants; })
		{
			parameters->RayTracingHitConstants = RayTracingHitUniformData{
			    .RayTracingHitInstanceCount = frame.PreparedScene.gpuBindings->RayTracing.InstanceCount,
			    .RayTracingHitMaterialCount = frame.PreparedScene.gpuBindings->RayTracing.MaterialCount};
		}
		if constexpr (requires { parameters->MaterialTextureTable; })
		{
			parameters->MaterialTextureTable = frame.PreparedScene.materialTextureTable.Binding;
		}
	}
}
