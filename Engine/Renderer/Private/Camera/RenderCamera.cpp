#include "PCH.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Scene/Camera/GameCamera.h"
#include "DepthConvention.h"

using namespace DirectX;

RenderCamera::RenderCamera(GameCamera& gameCamera) noexcept : m_gameCamera(gameCamera)
{
	RebuildMatrices();
}

void RenderCamera::Update() noexcept
{
	if (m_gameCamera.IsDirty())
	{
		RebuildMatrices();
		m_gameCamera.ClearDirty();
	}
}

void RenderCamera::ForceUpdate() noexcept
{
	RebuildMatrices();
	m_gameCamera.ClearDirty();
}

void RenderCamera::RebuildMatrices() noexcept
{
	const XMFLOAT3 position = m_gameCamera.GetPosition();
	const XMFLOAT3& direction = m_gameCamera.GetDirection();

	const XMVECTOR positionVec = XMLoadFloat3(&position);
	const XMVECTOR directionVec = XMLoadFloat3(&direction);
	const XMVECTOR targetVec = XMVectorAdd(positionVec, directionVec);

	const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	const XMMATRIX view = XMMatrixLookAtLH(positionVec, targetVec, worldUp);
	XMStoreFloat4x4(&m_viewMatrix, view);

	const float fovRadians = XMConvertToRadians(m_gameCamera.GetFovYDegrees());
	const float aspect = m_gameCamera.GetAspectRatio();
	const float nearZ = m_gameCamera.GetNearZ();
	const float farZ = m_gameCamera.GetFarZ();

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

XMFLOAT3 RenderCamera::GetPosition() const noexcept
{
	return m_gameCamera.GetPosition();
}

XMFLOAT3 RenderCamera::GetDirection() const noexcept
{
	return m_gameCamera.GetDirection();
}

float RenderCamera::GetNearZ() const noexcept
{
	return m_gameCamera.GetNearZ();
}

float RenderCamera::GetFarZ() const noexcept
{
	return m_gameCamera.GetFarZ();
}

PerViewConstantBufferData RenderCamera::GetViewConstantBufferData() const noexcept
{
	PerViewConstantBufferData data = {};

	data.ViewMTX = m_viewMatrix;
	data.ProjectionMTX = m_projectionMatrix;
	data.ViewProjMTX = m_viewProjMatrix;

	data.CameraPosition = m_gameCamera.GetPosition();
	data.CameraDirection = m_gameCamera.GetDirection();
	data.NearZ = m_gameCamera.GetNearZ();
	data.FarZ = m_gameCamera.GetFarZ();

	return data;
}
