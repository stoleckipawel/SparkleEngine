#include "PCH.h"
#include "Camera/RenderCamera.h"
#include "Config/DepthConvention.h"

using namespace DirectX;

RenderCamera::RenderCamera() noexcept
{
	RebuildMatrices(m_camera);
}

void RenderCamera::Update(const RenderCameraData& camera) noexcept
{
	RebuildMatrices(camera);
}

void RenderCamera::ForceUpdate(const RenderCameraData& camera) noexcept
{
	RebuildMatrices(camera);
}

void RenderCamera::RebuildMatrices(const RenderCameraData& camera) noexcept
{
	m_camera = camera;

	const XMFLOAT3 position = m_camera.Position;
	const XMFLOAT3& direction = m_camera.Direction;

	const XMVECTOR positionVec = XMLoadFloat3(&position);
	const XMVECTOR directionVec = XMLoadFloat3(&direction);
	const XMVECTOR targetVec = XMVectorAdd(positionVec, directionVec);

	const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	const XMMATRIX view = XMMatrixLookAtLH(positionVec, targetVec, worldUp);
	XMStoreFloat4x4(&m_viewMatrix, view);

	const float fovRadians = XMConvertToRadians(m_camera.FovYDegrees);
	const float aspect = m_camera.AspectRatio;
	const float nearZ = m_camera.NearZ;
	const float farZ = m_camera.FarZ;

	const XMMATRIX proj = DepthConvention::CreatePerspectiveFovLH(fovRadians, aspect, nearZ, farZ);
	XMStoreFloat4x4(&m_projectionMatrix, proj);

	const XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&m_viewProjMatrix, viewProj);

	m_frustum.ExtractFromViewProjection(m_viewProjMatrix);
}

XMMATRIX RenderCamera::GetViewMatrix() const noexcept
{
	return XMLoadFloat4x4(&m_viewMatrix);
}

XMMATRIX RenderCamera::GetProjectionMatrix() const noexcept
{
	return XMLoadFloat4x4(&m_projectionMatrix);
}

XMMATRIX RenderCamera::GetViewProjectionMatrix() const noexcept
{
	return XMLoadFloat4x4(&m_viewProjMatrix);
}

PerViewCameraConstantBufferData RenderCamera::GetCameraConstantBufferData() const noexcept
{
	PerViewCameraConstantBufferData data = {};

	data.ViewMTX = m_viewMatrix;
	data.ProjectionMTX = m_projectionMatrix;
	data.ViewProjMTX = m_viewProjMatrix;
	const XMMATRIX invView = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_viewMatrix));
	const XMMATRIX invProjection = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_projectionMatrix));
	XMStoreFloat4x4(&data.InvViewMTX, invView);
	XMStoreFloat4x4(&data.InvProjectionMTX, invProjection);

	data.Position = m_camera.Position;
	data.Direction = m_camera.Direction;
	data.NearZ = m_camera.NearZ;
	data.FarZ = m_camera.FarZ;

	return data;
}
