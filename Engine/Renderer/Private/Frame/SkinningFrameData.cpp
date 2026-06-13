#include "../PCH.h"

#include "Frame/SkinningFrameData.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Math/MathUtils.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"
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
	m_view = other.m_view;
	m_shaderResourceView = other.m_shaderResourceView;
	other.m_renderHardwareInterface = nullptr;
	other.m_buffer = {};
	other.m_view = {};
	other.m_shaderResourceView = {};
	return *this;
}

SkinningFrameData SkinningFrameData::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
{
	std::vector<JointMatrixData> matrices;
	matrices.reserve((std::max<std::size_t>)(sceneData.jointMatrices.size(), 1u));
	if (sceneData.jointMatrices.empty())
	{
		matrices.push_back(JointMatrixData{.SkinningMTX = MathUtils::IdentityFloat4x4()});
	}
	else
	{
		for (const DirectX::XMFLOAT4X4& matrix : sceneData.jointMatrices)
		{
			matrices.push_back(JointMatrixData{.SkinningMTX = matrix});
		}
	}

	RhiOwnedResourceHandle buffer = {};
	RhiResourceViewHandle view = {};
	const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBuffer(
	    matrices.data(),
	    matrices.size() * sizeof(JointMatrixData),
	    static_cast<std::uint32_t>(sizeof(JointMatrixData)),
	    L"SkinningJointMatrices",
	    buffer,
	    view);
	if (!created || !buffer || !view)
	{
		SPDLOG_LOGGER_WARN(
		    g_skinningFrameDataLogger,
		    "SkinningFrameData::Build: failed to upload {} joint matrices ({} bytes).",
		    matrices.size(),
		    matrices.size() * sizeof(JointMatrixData));
		return {};
	}

	SkinningFrameData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_buffer = buffer;
	frameData.m_view = view;
	frameData.m_shaderResourceView = renderHardwareInterface.GetDescriptorService().GetResourceViewGpuHandle(view);
	if (!frameData.m_shaderResourceView)
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
