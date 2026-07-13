#pragma once

#include "Frame/Core/FrameBufferResource.h"
#include "ShaderData/RenderViewLightingData.h"

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
	const FrameBufferResource& GetDirectionalLightsBuffer() const noexcept { return m_directionalLights; }
	const FrameBufferResource& GetPointLightsBuffer() const noexcept { return m_pointLights; }
	const FrameBufferResource& GetSpotLightsBuffer() const noexcept { return m_spotLights; }
	const FrameBufferResource& GetRectLightsBuffer() const noexcept { return m_rectLights; }

	static FrameLightingData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	ViewLightingData m_constants = {};
	FrameBufferResource m_directionalLights;
	FrameBufferResource m_pointLights;
	FrameBufferResource m_spotLights;
	FrameBufferResource m_rectLights;
};
