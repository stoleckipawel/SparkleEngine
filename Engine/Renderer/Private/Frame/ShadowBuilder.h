#pragma once

#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "RHI/Public/D3D12/Resources/D3D12ShadowConstantBufferData.h"
#include "RHI/Public/D3D12/Resources/D3D12ViewCameraConstantBufferData.h"

#include <array>
#include <d3d12.h>

struct RenderSceneData;

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

	ShadowBuildResult Build(const CameraSnapshot& mainCamera, const RenderSceneData& sceneData) const noexcept;

  private:
	static std::array<DirectX::XMFLOAT3, 8> BuildFrustumCorners(const CameraSnapshot& camera, float nearZ, float farZ) noexcept;
	static PerViewCameraConstantBufferData BuildLightCamera(
	    const std::array<DirectX::XMFLOAT3, 8>& frustumCorners,
	    const DirectX::XMFLOAT3& lightDirection) noexcept;
	static DirectX::XMVECTOR SelectUpVector(DirectX::FXMVECTOR forward) noexcept;
};