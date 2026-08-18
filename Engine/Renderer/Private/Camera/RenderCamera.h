#pragma once

#include "Rendering/RenderViewCameraData.h"

#include "ShaderData/RenderViewCameraData.h"
#include "Math/Frustum.h"
#include <DirectXMath.h>

class RenderCamera final
{
public:
	RenderCamera() noexcept;
	~RenderCamera() noexcept = default;

	RenderCamera(const RenderCamera&) = delete;
	RenderCamera& operator=(const RenderCamera&) = delete;
	RenderCamera(RenderCamera&&) = delete;
	RenderCamera& operator=(RenderCamera&&) = delete;

	void Update(const RenderViewCameraData& camera) noexcept;

	void ForceUpdate(const RenderViewCameraData& camera) noexcept;

	DirectX::XMMATRIX GetViewMatrix() const noexcept;
	DirectX::XMMATRIX GetProjectionMatrix() const noexcept;
	DirectX::XMMATRIX GetViewProjectionMatrix() const noexcept;

	const Frustum& GetFrustum() const noexcept { return m_frustum; }

	PerViewCameraConstantBufferData GetCameraConstantBufferData() const noexcept;
	float GetFovYDegrees() const noexcept { return m_camera.FovYDegrees; }

private:
	void RebuildMatrices(const RenderViewCameraData& camera) noexcept;

	RenderViewCameraData m_camera;
	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projectionMatrix;
	DirectX::XMFLOAT4X4 m_viewProjMatrix;
	Frustum m_frustum;
};
