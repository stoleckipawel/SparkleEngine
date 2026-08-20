#include "PCH.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"

bool RenderSceneGpuBufferBinding::IsValid() const noexcept
{
	return Resource && SizeInBytes > 0 && StrideInBytes > 0;
}

RenderSceneGpuBufferBinding::operator bool() const noexcept
{
	return IsValid();
}

bool RenderSceneGpuGeometryBindings::HasMeshInstanceBuffers() const noexcept
{
	return MeshInstances.IsValid() && MeshInstanceSlots.IsValid();
}

bool RenderSceneGpuGeometryBindings::HasSkinningBuffers() const noexcept
{
	return JointMatrices.IsValid() && PreviousJointMatrices.IsValid();
}

bool RenderSceneGpuGeometryBindings::HasMorphingBuffers() const noexcept
{
	return MorphWeights.IsValid() && PreviousMorphWeights.IsValid();
}

bool RenderSceneGpuRayTracingBindings::HasCompleteBuffers() const noexcept
{
	return Vertices && SkinInfluences && MorphTargetDeltas && Indices && Instances && Materials;
}
