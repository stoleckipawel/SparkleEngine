#pragma once

#include "Frame/Core/FrameContext.h"

namespace RayTracingHitDataPassBinding
{
	inline bool IsAvailable(const FrameContext& frame) noexcept
	{
		return frame.rayTracingHitData.IsValid() && frame.meshInstances.IsValid();
	}

	template <typename TParameterInstance>
	void SetParameters(TParameterInstance& parameters, const FrameContext& frame) noexcept
	{
		parameters->RayTracingHitVertices = frame.rayTracingHitData.GetVertexShaderResourceView();
		parameters->RayTracingHitIndices = frame.rayTracingHitData.GetIndexShaderResourceView();
		parameters->RayTracingHitInstances = frame.rayTracingHitData.GetInstanceShaderResourceView();
		parameters->RayTracingHitMaterials = frame.rayTracingHitData.GetMaterialShaderResourceView();
		parameters->MeshInstances = frame.meshInstances.GetShaderResourceView();
	}
}
