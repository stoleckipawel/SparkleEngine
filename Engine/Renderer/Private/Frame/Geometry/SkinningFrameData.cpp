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
	m_view = other.m_view;
	m_previousView = other.m_previousView;
	m_shaderResourceView = other.m_shaderResourceView;
	m_previousShaderResourceView = other.m_previousShaderResourceView;
	other.m_renderHardwareInterface = nullptr;
	other.m_buffer = {};
	other.m_previousBuffer = {};
	other.m_view = {};
	other.m_previousView = {};
	other.m_shaderResourceView = {};
	other.m_previousShaderResourceView = {};
	return *this;
}

SkinningFrameData SkinningFrameData::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
{
	std::vector<JointMatrixData> matrices;
	std::vector<JointMatrixData> previousMatrices;
	matrices.reserve((std::max<std::size_t>)(sceneData.jointMatrices.size(), 1u));
	previousMatrices.reserve((std::max<std::size_t>)(sceneData.previousJointMatrices.size(), 1u));
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
		    sceneData.previousJointMatrices.size() == sceneData.jointMatrices.size() ? sceneData.previousJointMatrices : sceneData.jointMatrices;
		for (const DirectX::XMFLOAT4X4& matrix : sourcePreviousMatrices)
		{
			previousMatrices.push_back(JointMatrixData{.SkinningMTX = matrix});
		}
	}

	RhiOwnedResourceHandle buffer = {};
	RhiOwnedResourceHandle previousBuffer = {};
	RhiResourceViewHandle view = {};
	RhiResourceViewHandle previousView = {};
	const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBuffer(
	    matrices.data(),
	    matrices.size() * sizeof(JointMatrixData),
	    static_cast<std::uint32_t>(sizeof(JointMatrixData)),
	    L"SkinningJointMatrices",
	    buffer,
	    view);
	const bool previousCreated = renderHardwareInterface.GetResourceService().CreateStructuredBuffer(
	    previousMatrices.data(),
	    previousMatrices.size() * sizeof(JointMatrixData),
	    static_cast<std::uint32_t>(sizeof(JointMatrixData)),
	    L"PreviousSkinningJointMatrices",
	    previousBuffer,
	    previousView);
	if (!created || !buffer || !view || !previousCreated || !previousBuffer || !previousView)
	{
		SPDLOG_LOGGER_WARN(
		    g_skinningFrameDataLogger,
		    "SkinningFrameData::Build: failed to upload {} joint matrices ({} bytes).",
		    matrices.size(),
		    matrices.size() * sizeof(JointMatrixData));
		if (view)
		{
			renderHardwareInterface.GetDescriptorService().ReleaseResourceView(view);
		}
		if (previousView)
		{
			renderHardwareInterface.GetDescriptorService().ReleaseResourceView(previousView);
		}
		if (buffer)
		{
			renderHardwareInterface.GetResourceService().ReleaseOwnedResource(buffer);
		}
		if (previousBuffer)
		{
			renderHardwareInterface.GetResourceService().ReleaseOwnedResource(previousBuffer);
		}
		return {};
	}

	SkinningFrameData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_buffer = buffer;
	frameData.m_previousBuffer = previousBuffer;
	frameData.m_view = view;
	frameData.m_previousView = previousView;
	frameData.m_shaderResourceView = renderHardwareInterface.GetDescriptorService().GetResourceViewGpuHandle(view);
	frameData.m_previousShaderResourceView = renderHardwareInterface.GetDescriptorService().GetResourceViewGpuHandle(previousView);
	if (!frameData.m_shaderResourceView || !frameData.m_previousShaderResourceView)
	{
		SPDLOG_LOGGER_WARN(g_skinningFrameDataLogger, "SkinningFrameData::Build: uploaded joint matrix buffer has no shader-resource descriptor.");
		return {};
	}
	return frameData;
}

void SkinningFrameData::Release() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_view)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_view);
		}
		if (m_previousView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_previousView);
		}
		if (m_buffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_buffer);
		}
		if (m_previousBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_previousBuffer);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_buffer = {};
	m_previousBuffer = {};
	m_view = {};
	m_previousView = {};
	m_shaderResourceView = {};
	m_previousShaderResourceView = {};
}
