#include "../../PCH.h"

#include "Frame/Lighting/FrameLightingData.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Lighting/LightingCVars.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <vector>

static const auto g_frameLightingDataLogger = Logging::GetOrCreateLogger("Renderer.FrameLightingData");

namespace
{
	template <typename TData>
	bool UploadLightBuffer(
	    RenderHardwareInterface& renderHardwareInterface,
	    const std::vector<TData>& source,
	    const wchar_t* debugName,
	    FrameBufferResource& outBuffer)
	{
		std::vector<TData> uploadData = source;
		if (uploadData.empty())
		{
			uploadData.push_back(TData{});
		}

		outBuffer.SizeInBytes = uploadData.size() * sizeof(TData);
		outBuffer.StrideInBytes = static_cast<std::uint32_t>(sizeof(TData));
		const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBufferResource(
		    uploadData.data(),
		    outBuffer.SizeInBytes,
		    outBuffer.StrideInBytes,
		    debugName,
		    outBuffer.Resource);
		return created && outBuffer.IsValid();
	}
}

FrameLightingData::~FrameLightingData() noexcept
{
	Release();
}

FrameLightingData::FrameLightingData(FrameLightingData&& other) noexcept
{
	*this = std::move(other);
}

FrameLightingData& FrameLightingData::operator=(FrameLightingData&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();
	m_renderHardwareInterface = other.m_renderHardwareInterface;
	m_constants = other.m_constants;
	m_directionalLights = other.m_directionalLights;
	m_pointLights = other.m_pointLights;
	m_spotLights = other.m_spotLights;
	m_rectLights = other.m_rectLights;

	other.m_renderHardwareInterface = nullptr;
	other.m_constants = {};
	other.m_directionalLights.Reset();
	other.m_pointLights.Reset();
	other.m_spotLights.Reset();
	other.m_rectLights.Reset();
	return *this;
}

FrameLightingData FrameLightingData::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
{
	const std::size_t directionalLightCount =
	    std::min(sceneData.directionalLights.size(), static_cast<std::size_t>(CVarMaxDirectionalLights.Get()));
	const std::size_t pointLightCount = std::min(sceneData.pointLights.size(), static_cast<std::size_t>(CVarMaxPointLights.Get()));
	const std::size_t spotLightCount = std::min(sceneData.spotLights.size(), static_cast<std::size_t>(CVarMaxSpotLights.Get()));
	const std::size_t rectLightCount = std::min(sceneData.rectLights.size(), static_cast<std::size_t>(CVarMaxRectLights.Get()));

	std::vector<DirectionalLightConstantBufferData> directionalLights;
	std::vector<PointLightConstantBufferData> pointLights;
	std::vector<SpotLightConstantBufferData> spotLights;
	std::vector<RectLightConstantBufferData> rectLights;
	directionalLights.reserve(directionalLightCount);
	pointLights.reserve(pointLightCount);
	spotLights.reserve(spotLightCount);
	rectLights.reserve(rectLightCount);

	for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.directionalLights[lightIndex];
		directionalLights.push_back(
		    DirectionalLightConstantBufferData{
		        .Direction = {sourceLight.direction.x, sourceLight.direction.y, sourceLight.direction.z},
		        .Intensity = sourceLight.intensity,
		        .Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z},
		        .AngularDiameter = sourceLight.angularDiameterRadians,
		        .CastShadow = sourceLight.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.pointLights[lightIndex];
		pointLights.push_back(
		    PointLightConstantBufferData{
		        .Position = {sourceLight.position.x, sourceLight.position.y, sourceLight.position.z},
		        .Range = sourceLight.range,
		        .Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z},
		        .Intensity = sourceLight.intensity,
		        .SourceRadius = sourceLight.sourceRadius,
		        .CastShadow = sourceLight.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.spotLights[lightIndex];
		spotLights.push_back(
		    SpotLightConstantBufferData{
		        .Position = {sourceLight.position.x, sourceLight.position.y, sourceLight.position.z},
		        .Range = sourceLight.range,
		        .Direction = {sourceLight.direction.x, sourceLight.direction.y, sourceLight.direction.z},
		        .InnerConeCosine = sourceLight.innerConeCosine,
		        .Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z},
		        .Intensity = sourceLight.intensity,
		        .OuterConeCosine = sourceLight.outerConeCosine,
		        .CastShadow = sourceLight.castShadow ? 1u : 0u,
		        .SourceRadius = sourceLight.sourceRadius});
	}

	for (std::size_t lightIndex = 0; lightIndex < rectLightCount; ++lightIndex)
	{
		const auto& sourceLight = sceneData.rectLights[lightIndex];
		rectLights.push_back(
		    RectLightConstantBufferData{
		        .Position = {sourceLight.position.x, sourceLight.position.y, sourceLight.position.z},
		        .Width = sourceLight.width,
		        .Direction = {sourceLight.direction.x, sourceLight.direction.y, sourceLight.direction.z},
		        .Height = sourceLight.height,
		        .Tangent = {sourceLight.tangent.x, sourceLight.tangent.y, sourceLight.tangent.z},
		        .Luminance = sourceLight.luminance,
		        .Color = {sourceLight.color.x, sourceLight.color.y, sourceLight.color.z},
		        .CastShadow = sourceLight.castShadow ? 1u : 0u});
	}

	FrameLightingData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_constants = ViewLightingData{
	    .DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount),
	    .PointLightCount = static_cast<std::uint32_t>(pointLightCount),
	    .SpotLightCount = static_cast<std::uint32_t>(spotLightCount),
	    .RectLightCount = static_cast<std::uint32_t>(rectLightCount)};

	const bool uploaded =
	    UploadLightBuffer(renderHardwareInterface, directionalLights, L"DirectionalLights", frameData.m_directionalLights) &&
	    UploadLightBuffer(renderHardwareInterface, pointLights, L"PointLights", frameData.m_pointLights) &&
	    UploadLightBuffer(renderHardwareInterface, spotLights, L"SpotLights", frameData.m_spotLights) &&
	    UploadLightBuffer(renderHardwareInterface, rectLights, L"RectLights", frameData.m_rectLights);
	if (!uploaded)
	{
		Diagnostics::Fail(g_frameLightingDataLogger, __FILE__, __LINE__, "FrameLightingData::Build: failed to upload lighting buffers.");
	}

	return frameData;
}

void FrameLightingData::Release() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_directionalLights)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_directionalLights.Resource);
		}
		if (m_pointLights)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_pointLights.Resource);
		}
		if (m_spotLights)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_spotLights.Resource);
		}
		if (m_rectLights)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_rectLights.Resource);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_constants = {};
	m_directionalLights.Reset();
	m_pointLights.Reset();
	m_spotLights.Reset();
	m_rectLights.Reset();
}
