#include "../../PCH.h"
#include "Frame/Geometry/MeshInstanceFrameData.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "RayTracing/Scene/RayTracingSceneFramePlan.h"
#include "SceneData/RenderSceneData.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"

#include <vector>

static const auto g_meshInstanceFrameDataLogger = Logging::GetOrCreateLogger("Renderer.MeshInstanceFrameData");

MeshInstanceFrameData::~MeshInstanceFrameData() noexcept
{
	Release();
}

MeshInstanceFrameData::MeshInstanceFrameData(MeshInstanceFrameData&& other) noexcept
{
	*this = std::move(other);
}

MeshInstanceFrameData& MeshInstanceFrameData::operator=(MeshInstanceFrameData&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();
	m_renderHardwareInterface = other.m_renderHardwareInterface;
	m_buffer = other.m_buffer;
	m_view = other.m_view;
	m_shaderResourceView = other.m_shaderResourceView;
	other.m_renderHardwareInterface = nullptr;
	other.m_buffer = {};
	other.m_view = {};
	other.m_shaderResourceView = {};
	return *this;
}

MeshInstanceFrameData MeshInstanceFrameData::Build(
    RenderHardwareInterface& renderHardwareInterface,
    const RenderSceneData& sceneData,
    const RayTracingSceneFramePlan* rayTracingFramePlan)
{
	if (sceneData.meshInstances.empty())
	{
		return {};
	}

	std::vector<MeshInstanceData> instances;
	instances.reserve(sceneData.meshInstances.size());
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		instances.push_back(
		    MeshInstanceData{
		        .WorldMTX = draw.Transform.WorldMatrix,
		        .PreviousWorldMTX = draw.Transform.PreviousWorldMatrix,
		        .WorldInvTransposeMTX = draw.Transform.WorldInvTranspose,
		        .MaterialSlot = draw.Material.Slot,
		        .Flags = draw.Geometry.MeshKind == RenderMeshKind::Skeletal && draw.Skinning.JointMatrixOffset != kInvalidMeshInstanceJointMatrixOffset
		                     ? MeshInstanceFlag_Skinned
		                     : 0u,
		        .JointMatrixOffset = draw.Skinning.JointMatrixOffset,
		        .PackedDebugData =
		            rayTracingFramePlan != nullptr
		                ? rayTracingFramePlan->MeshInstanceDebugData.GetPackedDebugVisualizationData(
		                      static_cast<std::uint32_t>(instances.size()))
		                : 0u});
	}

	RhiOwnedResourceHandle buffer = {};
	RhiResourceViewHandle view = {};
	const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBuffer(
	    instances.data(),
	    instances.size() * sizeof(MeshInstanceData),
	    static_cast<std::uint32_t>(sizeof(MeshInstanceData)),
	    L"MeshInstances",
	    buffer,
	    view);
	if (!created || !buffer || !view)
	{
		SPDLOG_LOGGER_WARN(
		    g_meshInstanceFrameDataLogger,
		    "MeshInstanceFrameData::Build: failed to upload {} mesh instance records ({} bytes).",
		    instances.size(),
		    instances.size() * sizeof(MeshInstanceData));
		return {};
	}

	MeshInstanceFrameData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_buffer = buffer;
	frameData.m_view = view;
	frameData.m_shaderResourceView = renderHardwareInterface.GetDescriptorService().GetResourceViewGpuHandle(view);
	if (!frameData.m_shaderResourceView)
	{
		SPDLOG_LOGGER_WARN(
		    g_meshInstanceFrameDataLogger,
		    "MeshInstanceFrameData::Build: uploaded mesh instance buffer has no shader-resource descriptor; instance batches will be skipped.");
		return {};
	}
	return frameData;
}

void MeshInstanceFrameData::Release() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_view)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_view);
		}
		if (m_buffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_buffer);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_buffer = {};
	m_view = {};
	m_shaderResourceView = {};
}
