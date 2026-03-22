#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"

#include "D3D12ConstantBufferData.h"
#include "Math/Frustum.h"
#include <DirectXMath.h>

class SPARKLE_RENDERER_API RenderCamera final
{
  public:
	RenderCamera() noexcept;
	~RenderCamera() noexcept = default;

	RenderCamera(const RenderCamera&) = delete;
	RenderCamera& operator=(const RenderCamera&) = delete;
	RenderCamera(RenderCamera&&) = delete;
	RenderCamera& operator=(RenderCamera&&) = delete;

	void Update(const CameraSnapshot& snapshot) noexcept;

	void ForceUpdate(const CameraSnapshot& snapshot) noexcept;

	DirectX::XMMATRIX GetViewMatrix() const noexcept;
	DirectX::XMMATRIX GetProjectionMatrix() const noexcept;
	DirectX::XMMATRIX GetViewProjectionMatrix() const noexcept;

	const Frustum& GetFrustum() const noexcept { return m_frustum; }

	PerViewCameraConstantBufferData GetCameraConstantBufferData() const noexcept;

  private:
	void RebuildMatrices(const CameraSnapshot& snapshot) noexcept;

	CameraSnapshot m_snapshot;
	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projectionMatrix;
	DirectX::XMFLOAT4X4 m_viewProjMatrix;
	Frustum m_frustum;
};
