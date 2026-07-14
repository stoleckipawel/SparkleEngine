#include "../../PCH.h"
#include "Frame/Geometry/MeshInstanceFrameData.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "SceneData/RenderSceneData.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <algorithm>
#include <utility>
#include <vector>

static const auto g_meshInstanceFrameDataLogger = Logging::GetOrCreateLogger("Renderer.MeshInstanceFrameData");

MeshInstanceFrameData MeshInstanceFrameData::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
{
	std::vector<MeshInstanceData> instances;
	instances.reserve(std::max<std::size_t>(sceneData.meshInstances.size(), 1));
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		instances.push_back(
		    MeshInstanceData{
		        .WorldMTX = draw.Transform.WorldMatrix,
		        .PreviousWorldMTX = draw.Transform.PreviousWorldMatrix,
		        .WorldInvTransposeMTX = draw.Transform.WorldInvTranspose,
		        .MaterialSlot = draw.Material.Slot,
		        .Flags = draw.Geometry.MeshKind == RenderMeshKind::Skeletal &&
		                         draw.Skinning.JointMatrixOffset != kInvalidMeshInstanceJointMatrixOffset
		                     ? MeshInstanceFlag_Skinned
		                     : 0u,
		        .JointMatrixOffset = draw.Skinning.JointMatrixOffset,
		        .DebugData = static_cast<std::uint32_t>(instances.size())});
	}
	if (instances.empty())
	{
		// Keep the graph binding valid for an empty scene. No draw indexes this sentinel.
		instances.emplace_back();
	}

	FrameBufferResource buffer = FrameBufferResource::Upload(
	    renderHardwareInterface.GetResourceService(),
	    instances.data(),
	    instances.size() * sizeof(MeshInstanceData),
	    static_cast<std::uint32_t>(sizeof(MeshInstanceData)),
	    L"MeshInstances");
	if (!buffer)
	{
		SPDLOG_LOGGER_WARN(
		    g_meshInstanceFrameDataLogger,
		    "MeshInstanceFrameData::Build: failed to upload {} mesh instance records ({} bytes).",
		    instances.size(),
		    instances.size() * sizeof(MeshInstanceData));
		return {};
	}

	MeshInstanceFrameData frameData;
	frameData.m_buffer = std::move(buffer);
	return frameData;
}
