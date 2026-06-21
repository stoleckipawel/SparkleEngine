#pragma once

#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Pipeline/PassBindingOverrides.h"
#include "RayTracing/RayTracingHitData.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"

#include <cstdint>

namespace RayTracingHitDataPassBinding
{
	template <typename TData>
	FrameGraphBufferHandle CreatePlaceholderBuffer(FrameGraphBuilder& builder, const char* name)
	{
		return builder.CreateBuffer(FrameGraphBufferDesc::Create(name, sizeof(TData), static_cast<std::uint32_t>(sizeof(TData))));
	}

	template <typename TParameterInstance>
	void DeclareResources(FrameGraphBuilder& builder, TParameterInstance& parameters)
	{
		parameters->RayTracingHitVertices =
		    builder.CreateSRV<RayTracingHitVertex>(CreatePlaceholderBuffer<RayTracingHitVertex>(builder, "RayTracingHitVerticesPlaceholder"));
		parameters->RayTracingHitIndices =
		    builder.CreateSRV<std::uint32_t>(CreatePlaceholderBuffer<std::uint32_t>(builder, "RayTracingHitIndicesPlaceholder"));
		parameters->RayTracingHitInstances =
		    builder.CreateSRV<RayTracingHitInstance>(CreatePlaceholderBuffer<RayTracingHitInstance>(builder, "RayTracingHitInstancesPlaceholder"));
		parameters->RayTracingHitMaterials =
		    builder.CreateSRV<RayTracingHitMaterial>(CreatePlaceholderBuffer<RayTracingHitMaterial>(builder, "RayTracingHitMaterialsPlaceholder"));
		parameters->MeshInstances =
		    builder.CreateSRV<MeshInstanceData>(CreatePlaceholderBuffer<MeshInstanceData>(builder, "RayTracingMeshInstancesPlaceholder"));
	}

	inline bool IsAvailable(const FrameContext& frame) noexcept
	{
		return frame.rayTracingHitData.IsValid() && frame.meshInstances.IsValid();
	}

	inline void Bind(PassBindingOverrides& overrides, const FrameContext& frame) noexcept
	{
		overrides.SetDescriptorTable("RayTracingHitVertices", frame.rayTracingHitData.GetVertexShaderResourceView());
		overrides.SetDescriptorTable("RayTracingHitIndices", frame.rayTracingHitData.GetIndexShaderResourceView());
		overrides.SetDescriptorTable("RayTracingHitInstances", frame.rayTracingHitData.GetInstanceShaderResourceView());
		overrides.SetDescriptorTable("RayTracingHitMaterials", frame.rayTracingHitData.GetMaterialShaderResourceView());
		overrides.SetDescriptorTable("MeshInstances", frame.meshInstances.GetShaderResourceView());
	}
}
