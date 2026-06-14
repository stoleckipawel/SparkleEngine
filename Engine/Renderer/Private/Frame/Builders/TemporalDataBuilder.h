#pragma once

#include "RHI/Public/Resources/PerTemporalConstantBufferData.h"
#include "RHI/Public/Resources/RenderViewCameraData.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <DirectXMath.h>
#include <string>
#include <string_view>

class RenderCamera;

class TemporalDataBuilder final
{
  public:
	void ResetHistory(std::string_view reason) noexcept;
	PerTemporalConstantBufferData BuildTemporalData(
	    const RenderCamera& renderCamera,
	    const PerViewCameraConstantBufferData& cameraData,
	    const RhiViewport& viewport) noexcept;

  private:
	struct CameraPose
	{
		DirectX::XMFLOAT4X4 ViewMTX = {};
		DirectX::XMFLOAT4X4 ProjectionMTX = {};
		DirectX::XMFLOAT4X4 ViewProjMTX = {};
		DirectX::XMFLOAT3 Position = {};
		DirectX::XMFLOAT3 Direction = {0.0f, 0.0f, 1.0f};
		float FovYDegrees = 60.0f;
	};

	static CameraPose CapturePose(const RenderCamera& renderCamera, const PerViewCameraConstantBufferData& cameraData) noexcept;
	static bool IsLikelyCameraCut(const CameraPose& previousPose, const CameraPose& currentPose) noexcept;

	CameraPose m_previousPose;
	DirectX::XMFLOAT2 m_previousJitter = {};
	uint32_t m_jitterIndex = 0;
	bool m_hasPreviousPose = false;
	bool m_resetRequested = false;
	std::string m_resetReason;
};
