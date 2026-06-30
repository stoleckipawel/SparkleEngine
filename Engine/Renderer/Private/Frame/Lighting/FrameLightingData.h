#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "RHI/Public/Resources/RhiResourceView.h"

class RenderHardwareInterface;
struct RenderSceneData;

class FrameLightingData final
{
  public:
	FrameLightingData() noexcept = default;
	~FrameLightingData() noexcept;

	FrameLightingData(const FrameLightingData&) = delete;
	FrameLightingData& operator=(const FrameLightingData&) = delete;
	FrameLightingData(FrameLightingData&& other) noexcept;
	FrameLightingData& operator=(FrameLightingData&& other) noexcept;

	const ViewLightingData& GetConstants() const noexcept { return m_constants; }
	RhiGpuDescriptorHandle GetDirectionalLightsShaderResourceView() const noexcept { return m_directionalLightsShaderResourceView; }
	RhiGpuDescriptorHandle GetPointLightsShaderResourceView() const noexcept { return m_pointLightsShaderResourceView; }
	RhiGpuDescriptorHandle GetSpotLightsShaderResourceView() const noexcept { return m_spotLightsShaderResourceView; }
	RhiGpuDescriptorHandle GetRectLightsShaderResourceView() const noexcept { return m_rectLightsShaderResourceView; }

	static FrameLightingData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	ViewLightingData m_constants = {};
	RhiOwnedResourceHandle m_directionalLightsBuffer = {};
	RhiOwnedResourceHandle m_pointLightsBuffer = {};
	RhiOwnedResourceHandle m_spotLightsBuffer = {};
	RhiOwnedResourceHandle m_rectLightsBuffer = {};
	RhiResourceViewHandle m_directionalLightsView = {};
	RhiResourceViewHandle m_pointLightsView = {};
	RhiResourceViewHandle m_spotLightsView = {};
	RhiResourceViewHandle m_rectLightsView = {};
	RhiGpuDescriptorHandle m_directionalLightsShaderResourceView = {};
	RhiGpuDescriptorHandle m_pointLightsShaderResourceView = {};
	RhiGpuDescriptorHandle m_spotLightsShaderResourceView = {};
	RhiGpuDescriptorHandle m_rectLightsShaderResourceView = {};
};
