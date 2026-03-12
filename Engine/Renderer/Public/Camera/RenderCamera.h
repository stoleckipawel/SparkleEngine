// ============================================================================
// RenderCamera.h
// ----------------------------------------------------------------------------
// Rendering-side camera representation. Reads from GameCamera and builds
// view/projection matrices for GPU submission.
//
#pragma once

#include "Renderer/Public/RendererAPI.h"

#include "D3D12ConstantBufferData.h"
#include "Math/Frustum.h"
#include <DirectXMath.h>

class GameCamera;

// ============================================================================
// RenderCamera
// ============================================================================

class SPARKLE_RENDERER_API RenderCamera final
{
  public:
	explicit RenderCamera(GameCamera& gameCamera) noexcept;
	~RenderCamera() noexcept = default;

	RenderCamera(const RenderCamera&) = delete;
	RenderCamera& operator=(const RenderCamera&) = delete;
	RenderCamera(RenderCamera&&) = delete;
	RenderCamera& operator=(RenderCamera&&) = delete;

	/// Call each frame before rendering.
	void Update() noexcept;

	/// Forces a full matrix rebuild regardless of dirty state.
	void ForceUpdate() noexcept;

	DirectX::XMMATRIX GetViewMatrix() const noexcept;
	DirectX::XMMATRIX GetProjectionMatrix() const noexcept;
	DirectX::XMMATRIX GetViewProjectionMatrix() const noexcept;

	const Frustum& GetFrustum() const noexcept { return m_frustum; }

	/// Camera transform data (cached from GameCamera).
	DirectX::XMFLOAT3 GetPosition() const noexcept;
	DirectX::XMFLOAT3 GetDirection() const noexcept;
	float GetNearZ() const noexcept;
	float GetFarZ() const noexcept;

	PerViewConstantBufferData GetViewConstantBufferData() const noexcept;

  private:
	void RebuildMatrices() noexcept;

	GameCamera& m_gameCamera;

	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projectionMatrix;
	DirectX::XMFLOAT4X4 m_viewProjMatrix;
	Frustum m_frustum;
};
