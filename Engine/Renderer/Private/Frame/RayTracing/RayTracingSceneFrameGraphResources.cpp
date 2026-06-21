#include "../../PCH.h"

#include "Frame/RayTracing/RayTracingSceneFrameGraphResources.h"

bool RayTracingSceneFrameGraphResources::HasSceneTlas() const noexcept
{
	return SceneTlas.IsValid();
}

bool RayTracingSceneFrameGraphResources::HasPartitionedTlasResources() const noexcept
{
	return PtlasLogicalUpdateRecords.IsValid() && PtlasNativeOperationData.IsValid() && PtlasScratch.IsValid();
}
