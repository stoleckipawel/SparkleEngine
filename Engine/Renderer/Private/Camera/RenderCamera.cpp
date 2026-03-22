#include "PCH.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "DepthConvention.h"

using namespace DirectX;

RenderCamera::RenderCamera() noexcept
{
	RebuildMatrices(m_snapshot);
}

void RenderCamera::Update(const CameraSnapshot& snapshot) noexcept
{
	RebuildMatrices(snapshot);
}

void RenderCamera::ForceUpdate(const CameraSnapshot& snapshot) noexcept
{
	RebuildMatrices(snapshot);
}

void RenderCamera::RebuildMatrices(const CameraSnapshot& snapshot) noexcept
{
	m_snapshot = snapshot;

	const XMFLOAT3 position = m_snapshot.position;
	const XMFLOAT3& direction = m_snapshot.direction;

	const XMVECTOR positionVec = XMLoadFloat3(&position);
	const XMVECTOR directionVec = XMLoadFloat3(&direction);
	const XMVECTOR targetVec = XMVectorAdd(positionVec, directionVec);

	const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	const XMMATRIX view = XMMatrixLookAtLH(positionVec, targetVec, worldUp);
	XMStoreFloat4x4(&m_viewMatrix, view);

	const float fovRadians = XMConvertToRadians(m_snapshot.fovYDegrees);
	const float aspect = m_snapshot.aspectRatio;
	const float nearZ = m_snapshot.nearZ;
	const float farZ = m_snapshot.farZ;

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

	data.Position = m_snapshot.position;
	data.Direction = m_snapshot.direction;
	data.NearZ = m_snapshot.nearZ;
	data.FarZ = m_snapshot.farZ;

	return data;
}
