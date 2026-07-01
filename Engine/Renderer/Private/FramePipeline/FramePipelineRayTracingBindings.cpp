#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Frame/RayTracing/RayTracingSceneFrameData.h"
#include "FrameGraph/FrameGraph.h"

void FramePipeline::BindRayTracingFrameGraphResources(const RayTracingSceneFrameData& rayTracingScene) noexcept
{
	if (m_frameGraph == nullptr)
	{
		return;
	}

	if (rayTracingScene.PtlasFrameGraphResources.HasLogicalUpdateRecords())
	{
		m_frameGraph->BindPersistentBuffer(
		    m_frameResources.Persistent.RayTracing.PtlasLogicalUpdateRecords,
		    rayTracingScene.PtlasFrameGraphResources.LogicalUpdateRecords);
	}
	else
	{
		m_frameGraph->ClearPersistentBufferBinding(m_frameResources.Persistent.RayTracing.PtlasLogicalUpdateRecords);
	}

	if (rayTracingScene.PtlasFrameGraphResources.HasNativeOperationData())
	{
		m_frameGraph->BindPersistentBuffer(
		    m_frameResources.Persistent.RayTracing.PtlasNativeOperationData,
		    rayTracingScene.PtlasFrameGraphResources.NativeOperationData);
	}
	else
	{
		m_frameGraph->ClearPersistentBufferBinding(m_frameResources.Persistent.RayTracing.PtlasNativeOperationData);
	}

	if (rayTracingScene.PtlasFrameGraphResources.HasScratch())
	{
		m_frameGraph->BindPersistentBuffer(
		    m_frameResources.Persistent.RayTracing.PtlasScratch,
		    rayTracingScene.PtlasFrameGraphResources.Scratch);
	}
	else
	{
		m_frameGraph->ClearPersistentBufferBinding(m_frameResources.Persistent.RayTracing.PtlasScratch);
	}
}

void FramePipeline::ClearRayTracingFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr)
	{
		return;
	}

	m_frameGraph->ClearPersistentBufferBinding(m_frameResources.Persistent.RayTracing.PtlasLogicalUpdateRecords);
	m_frameGraph->ClearPersistentBufferBinding(m_frameResources.Persistent.RayTracing.PtlasNativeOperationData);
	m_frameGraph->ClearPersistentBufferBinding(m_frameResources.Persistent.RayTracing.PtlasScratch);
}
