#include "../../PCH.h"

#include "GpuScene/GpuSceneSkinning.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Math/MathUtils.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <utility>
#include <vector>

static const auto g_gpuSceneSkinningLogger = Logging::GetOrCreateLogger("Renderer.GpuScene.Skinning");

GpuSceneSkinning GpuSceneSkinning::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
{
	std::vector<JointMatrixData> matrices;
	std::vector<JointMatrixData> previousMatrices;
	matrices.reserve((std::max<std::size_t>) (sceneData.jointMatrices.size(), 1u));
	previousMatrices.reserve((std::max<std::size_t>) (sceneData.previousJointMatrices.size(), 1u));
	if (sceneData.jointMatrices.empty())
	{
		matrices.push_back(JointMatrixData{.SkinningMTX = MathUtils::IdentityFloat4x4()});
		previousMatrices.push_back(JointMatrixData{.SkinningMTX = MathUtils::IdentityFloat4x4()});
	}
	else
	{
		for (const DirectX::XMFLOAT4X4& matrix : sceneData.jointMatrices)
		{
			matrices.push_back(JointMatrixData{.SkinningMTX = matrix});
		}
		const std::vector<DirectX::XMFLOAT4X4>& sourcePreviousMatrices =
		    sceneData.previousJointMatrices.size() == sceneData.jointMatrices.size() ? sceneData.previousJointMatrices
		                                                                             : sceneData.jointMatrices;
		for (const DirectX::XMFLOAT4X4& matrix : sourcePreviousMatrices)
		{
			previousMatrices.push_back(JointMatrixData{.SkinningMTX = matrix});
		}
	}

	GpuSceneBuffer buffer = GpuSceneBuffer::Upload(
	    renderHardwareInterface.GetResourceService(),
	    matrices.data(),
	    matrices.size() * sizeof(JointMatrixData),
	    static_cast<std::uint32_t>(sizeof(JointMatrixData)),
	    L"SkinningJointMatrices");
		
	GpuSceneBuffer previousBuffer = GpuSceneBuffer::Upload(
	    renderHardwareInterface.GetResourceService(),
	    previousMatrices.data(),
	    previousMatrices.size() * sizeof(JointMatrixData),
	    static_cast<std::uint32_t>(sizeof(JointMatrixData)),
	    L"PreviousSkinningJointMatrices");

	if (!buffer || !previousBuffer)
	{
		SPDLOG_LOGGER_WARN(
		    g_gpuSceneSkinningLogger,
		    "GpuSceneSkinning::Build: failed to upload {} joint matrices ({} bytes).",
		    matrices.size(),
		    matrices.size() * sizeof(JointMatrixData));
		return {};
	}

	GpuSceneSkinning frameData;
	frameData.m_buffer = std::move(buffer);
	frameData.m_previousBuffer = std::move(previousBuffer);
	return frameData;
}
