#pragma once

#include "Frame/Core/FrameContext.h"

namespace RayTracingHitDataPassBinding
{
	inline bool IsAvailable(const FrameContext& frame) noexcept
	{
		return frame.rayTracingHitData.IsValid() && frame.meshInstances.IsValid() && frame.skinning.IsValid();
	}

	inline bool HasTriangleMaterialData(const FrameContext& frame) noexcept
	{
		return frame.rayTracingHitData.IsValid();
	}

	template <typename TParameterInstance>
	void SetTriangleMaterialParameters(TParameterInstance& parameters, const FrameContext& frame) noexcept
	{
		parameters->RayTracingHitVertices = frame.rayTracingHitData.GetVertexShaderResourceView();
		parameters->RayTracingHitIndices = frame.rayTracingHitData.GetIndexShaderResourceView();
		parameters->RayTracingHitInstances = frame.rayTracingHitData.GetInstanceShaderResourceView();
		parameters->RayTracingHitMaterials = frame.rayTracingHitData.GetMaterialShaderResourceView();
	}

	template <typename TParameterInstance>
	void SetParameters(TParameterInstance& parameters, const FrameContext& frame) noexcept
	{
		SetTriangleMaterialParameters(parameters, frame);
		parameters->MeshInstances = frame.meshInstances.GetShaderResourceView();
		parameters->SkinInfluences = frame.rayTracingHitData.GetSkinInfluenceShaderResourceView();
		parameters->JointMatrices = frame.skinning.GetShaderResourceView();
	}

	template <typename TParameterInstance>
	void SetTemporalSurfaceParameters(TParameterInstance& parameters, const FrameContext& frame) noexcept
	{
		SetParameters(parameters, frame);
		parameters->PreviousJointMatrices = frame.skinning.GetPreviousShaderResourceView();
	}
}
