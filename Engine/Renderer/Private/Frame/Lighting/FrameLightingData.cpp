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
	    RhiOwnedResourceHandle& outBuffer,
	    RhiResourceViewHandle& outView,
	    RhiGpuDescriptorHandle& outShaderResourceView)
	{
		std::vector<TData> uploadData = source;
		if (uploadData.empty())
		{
			uploadData.push_back(TData{});
		}

		const bool created = renderHardwareInterface.GetResourceService().CreateStructuredBuffer(
		    uploadData.data(),
		    uploadData.size() * sizeof(TData),
		    static_cast<std::uint32_t>(sizeof(TData)),
		    debugName,
		    outBuffer,
		    outView);
		if (!created || !outBuffer || !outView)
		{
			return false;
		}

		outShaderResourceView = renderHardwareInterface.GetDescriptorService().GetResourceViewGpuHandle(outView);
		return static_cast<bool>(outShaderResourceView);
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
	m_directionalLightsBuffer = other.m_directionalLightsBuffer;
	m_pointLightsBuffer = other.m_pointLightsBuffer;
	m_spotLightsBuffer = other.m_spotLightsBuffer;
	m_directionalLightsView = other.m_directionalLightsView;
	m_pointLightsView = other.m_pointLightsView;
	m_spotLightsView = other.m_spotLightsView;
	m_directionalLightsShaderResourceView = other.m_directionalLightsShaderResourceView;
	m_pointLightsShaderResourceView = other.m_pointLightsShaderResourceView;
	m_spotLightsShaderResourceView = other.m_spotLightsShaderResourceView;

	other.m_renderHardwareInterface = nullptr;
	other.m_constants = {};
	other.m_directionalLightsBuffer = {};
	other.m_pointLightsBuffer = {};
	other.m_spotLightsBuffer = {};
	other.m_directionalLightsView = {};
	other.m_pointLightsView = {};
	other.m_spotLightsView = {};
	other.m_directionalLightsShaderResourceView = {};
	other.m_pointLightsShaderResourceView = {};
	other.m_spotLightsShaderResourceView = {};
	return *this;
}

FrameLightingData FrameLightingData::Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData)
{
	const std::size_t directionalLightCount =
	    std::min(sceneData.directionalLights.size(), static_cast<std::size_t>(CVarMaxDirectionalLights.Get()));
	const std::size_t pointLightCount = std::min(sceneData.pointLights.size(), static_cast<std::size_t>(CVarMaxPointLights.Get()));
	const std::size_t spotLightCount = std::min(sceneData.spotLights.size(), static_cast<std::size_t>(CVarMaxSpotLights.Get()));

	std::vector<DirectionalLightConstantBufferData> directionalLights;
	std::vector<PointLightConstantBufferData> pointLights;
	std::vector<SpotLightConstantBufferData> spotLights;
	directionalLights.reserve(directionalLightCount);
	pointLights.reserve(pointLightCount);
	spotLights.reserve(spotLightCount);

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

	FrameLightingData frameData;
	frameData.m_renderHardwareInterface = &renderHardwareInterface;
	frameData.m_constants = ViewLightingData{
	    .DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount),
	    .PointLightCount = static_cast<std::uint32_t>(pointLightCount),
	    .SpotLightCount = static_cast<std::uint32_t>(spotLightCount)};

	const bool uploaded =
	    UploadLightBuffer(
	        renderHardwareInterface,
	        directionalLights,
	        L"DirectionalLights",
	        frameData.m_directionalLightsBuffer,
	        frameData.m_directionalLightsView,
	        frameData.m_directionalLightsShaderResourceView) &&
	    UploadLightBuffer(
	        renderHardwareInterface,
	        pointLights,
	        L"PointLights",
	        frameData.m_pointLightsBuffer,
	        frameData.m_pointLightsView,
	        frameData.m_pointLightsShaderResourceView) &&
	    UploadLightBuffer(
	        renderHardwareInterface,
	        spotLights,
	        L"SpotLights",
	        frameData.m_spotLightsBuffer,
	        frameData.m_spotLightsView,
	        frameData.m_spotLightsShaderResourceView);
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
		if (m_directionalLightsView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_directionalLightsView);
		}
		if (m_pointLightsView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_pointLightsView);
		}
		if (m_spotLightsView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_spotLightsView);
		}
		if (m_directionalLightsBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_directionalLightsBuffer);
		}
		if (m_pointLightsBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_pointLightsBuffer);
		}
		if (m_spotLightsBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_spotLightsBuffer);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_constants = {};
	m_directionalLightsBuffer = {};
	m_pointLightsBuffer = {};
	m_spotLightsBuffer = {};
	m_directionalLightsView = {};
	m_pointLightsView = {};
	m_spotLightsView = {};
	m_directionalLightsShaderResourceView = {};
	m_pointLightsShaderResourceView = {};
	m_spotLightsShaderResourceView = {};
}
