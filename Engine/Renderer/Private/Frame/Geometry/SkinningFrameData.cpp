#include "../../PCH.h"

#include "Frame/Geometry/SkinningFrameData.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Math/MathUtils.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <vector>

static const auto g_skinningFrameDataLogger = Logging::GetOrCreateLogger("Renderer.SkinningFrameData");

SkinningFrameData::~SkinningFrameData() noexcept
{
	Release();
}

SkinningFrameData::SkinningFrameData(SkinningFrameData&& other) noexcept
{
	*this = std::move(other);
}

SkinningFrameData& SkinningFrameData::operator=(SkinningFrameData&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();
	m_renderHardwareInterface = other.m_renderHardwareInterface;
	m_buffer = other.m_buffer;
	m_previousBuffer = other.m_previousBuffer;
	other.m_renderHardwareInterface = nullptr;
	other.m_buffer.Reset();
	other.m_previousBuffer.Reset();
	return *this;
}

SkinningFrameData SkinningFrameData::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
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

	FrameBufferResource buffer{
	    .SizeInBytes = matrices.size() * sizeof(JointMatrixData),
	    .StrideInBytes = static_cast<std::uint32_t>(sizeof(JointMatrixData))};
	FrameBufferResource previousBuffer{
	    .SizeInBytes = previousMatrices.size() * sizeof(JointMatrixData),
	    .StrideInBytes = static_cast<std::uint32_t>(sizeof(JointMatrixData))};
	const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBufferResource(
	    matrices.data(),
	    buffer.SizeInBytes,
	    buffer.StrideInBytes,
	    L"SkinningJointMatrices",
	    buffer.Resource);
	const bool previousCreated = renderHardwareInterface.GetResourceService().CreateStructuredBufferResource(
	    previousMatrices.data(),
	    previousBuffer.SizeInBytes,
	    previousBuffer.StrideInBytes,
	    L"PreviousSkinningJointMatrices",
	    previousBuffer.Resource);
	if (!created || !buffer || !previousCreated || !previousBuffer)
	{
		SPDLOG_LOGGER_WARN(
		    g_skinningFrameDataLogger,
		    "SkinningFrameData::Build: failed to upload {} joint matrices ({} bytes).",
		    matrices.size(),
		    matrices.size() * sizeof(JointMatrixData));
		if (buffer)
		{
			renderHardwareInterface.GetResourceService().ReleaseOwnedResource(buffer.Resource);
		}
		if (previousBuffer)
		{
			renderHardwareInterface.GetResourceService().ReleaseOwnedResource(previousBuffer.Resource);
		}
		return {};
	}

	SkinningFrameData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_buffer = buffer;
	frameData.m_previousBuffer = previousBuffer;
	return frameData;
}

void SkinningFrameData::Release() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_buffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_buffer.Resource);
		}
		if (m_previousBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_previousBuffer.Resource);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_buffer.Reset();
	m_previousBuffer.Reset();
}
