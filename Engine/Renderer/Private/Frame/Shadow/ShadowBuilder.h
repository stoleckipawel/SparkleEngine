#pragma once

#include "Scene/Camera/CameraSnapshot.h"
#include "RHI/Public/D3D12/Resources/D3D12ShadowConstantBufferData.h"
#include "RHI/Public/D3D12/Resources/D3D12ViewCameraConstantBufferData.h"

#include <d3d12.h>

struct ShadowBuildResult
{
	ShadowConstantBufferData shadow = {};
	PerViewCameraConstantBufferData cameraData = {};
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};
};

class ShadowBuilder final
{
  public:
	ShadowBuilder() noexcept = default;
	~ShadowBuilder() noexcept = default;

	ShadowBuilder(const ShadowBuilder&) = delete;
	ShadowBuilder& operator=(const ShadowBuilder&) = delete;
	ShadowBuilder(ShadowBuilder&&) = delete;
	ShadowBuilder& operator=(ShadowBuilder&&) = delete;

	ShadowBuildResult Build(
	    const CameraSnapshot& mainCamera,
	    const DirectX::XMFLOAT3& lightDirection,
	    float cascadeNearZ,
	    float cascadeFarZ) const noexcept;

  private:
	static PerViewCameraConstantBufferData BuildLightCameraForSphere(
	    const DirectX::XMFLOAT3& sphereCenter,
	    float sphereRadius,
	    const DirectX::XMFLOAT3& lightDirection) noexcept;
	static DirectX::XMVECTOR SelectUpVector(DirectX::FXMVECTOR forward) noexcept;
};