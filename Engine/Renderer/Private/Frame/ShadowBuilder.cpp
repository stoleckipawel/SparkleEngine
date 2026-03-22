#include "PCH.h"

#include "Frame/ShadowBuilder.h"

#include "Renderer/Public/DepthConvention.h"
#include "RHI/Public/RenderConfig.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

using namespace DirectX;

ShadowBuildResult ShadowBuilder::Build(const CameraSnapshot& mainCamera, const XMFLOAT3& lightDirection) const noexcept
{
	ShadowBuildResult result{};
	result.shadow.ShadowMapSize = static_cast<float>(RenderConfig::Shadows::ShadowMapResolution);
	result.shadow.DepthBias = RenderConfig::Shadows::DepthBias;
	result.shadow.NormalBias = RenderConfig::Shadows::NormalBias;
	result.viewport = D3D12_VIEWPORT{
	    0.0f, 0.0f,
	    static_cast<float>(RenderConfig::Shadows::ShadowMapResolution),
	    static_cast<float>(RenderConfig::Shadows::ShadowMapResolution),
	    0.0f, 1.0f};
	result.scissorRect = D3D12_RECT{
	    0, 0,
	    static_cast<LONG>(RenderConfig::Shadows::ShadowMapResolution),
	    static_cast<LONG>(RenderConfig::Shadows::ShadowMapResolution)};

	const float shadowDistance = std::min(mainCamera.farZ, RenderConfig::Shadows::ShadowDistance);
	const std::array<XMFLOAT3, 8> frustumCorners = BuildFrustumCorners(mainCamera, mainCamera.nearZ, shadowDistance);
	result.cameraData = BuildLightCamera(frustumCorners, lightDirection);
	result.shadow.ViewProjMTX = result.cameraData.ViewProjMTX;

	return result;
}

std::array<XMFLOAT3, 8> ShadowBuilder::BuildFrustumCorners(const CameraSnapshot& camera, float nearZ, float farZ) noexcept
{
	const XMVECTOR cameraPosition = XMLoadFloat3(&camera.position);
	const XMVECTOR cameraForward = XMVector3Normalize(XMLoadFloat3(&camera.direction));
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Cross(up, cameraForward);
	if (XMVectorGetX(XMVector3LengthSq(right)) < 1.0e-6f)
	{
		up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		right = XMVector3Cross(up, cameraForward);
	}

	right = XMVector3Normalize(right);
	up = XMVector3Normalize(XMVector3Cross(cameraForward, right));

	const float tanHalfFovY = std::tan(XMConvertToRadians(camera.fovYDegrees) * 0.5f);
	const float nearHeight = nearZ * tanHalfFovY;
	const float nearWidth = nearHeight * camera.aspectRatio;
	const float farHeight = farZ * tanHalfFovY;
	const float farWidth = farHeight * camera.aspectRatio;

	const XMVECTOR nearCenter = cameraPosition + cameraForward * nearZ;
	const XMVECTOR farCenter = cameraPosition + cameraForward * farZ;

	std::array<XMFLOAT3, 8> corners{};
	XMStoreFloat3(&corners[0], nearCenter - right * nearWidth - up * nearHeight);
	XMStoreFloat3(&corners[1], nearCenter - right * nearWidth + up * nearHeight);
	XMStoreFloat3(&corners[2], nearCenter + right * nearWidth + up * nearHeight);
	XMStoreFloat3(&corners[3], nearCenter + right * nearWidth - up * nearHeight);
	XMStoreFloat3(&corners[4], farCenter - right * farWidth - up * farHeight);
	XMStoreFloat3(&corners[5], farCenter - right * farWidth + up * farHeight);
	XMStoreFloat3(&corners[6], farCenter + right * farWidth + up * farHeight);
	XMStoreFloat3(&corners[7], farCenter + right * farWidth - up * farHeight);
	return corners;
}

PerViewCameraConstantBufferData ShadowBuilder::BuildLightCamera(
    const std::array<XMFLOAT3, 8>& frustumCorners,
    const XMFLOAT3& lightDirection) noexcept
{
	XMVECTOR center = XMVectorZero();
	for (std::size_t i = 0; i < frustumCorners.size(); ++i)
	{
		center += XMLoadFloat3(&frustumCorners[i]);
	}
	center /= static_cast<float>(frustumCorners.size());

	XMVECTOR lightForward = XMVector3Normalize(XMLoadFloat3(&lightDirection));
	if (XMVectorGetX(XMVector3LengthSq(lightForward)) < 1.0e-6f)
	{
		lightForward = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	}

	const XMVECTOR lightUp = SelectUpVector(lightForward);

	float radius = 0.0f;
	for (std::size_t i = 0; i < frustumCorners.size(); ++i)
	{
		const float dist = XMVectorGetX(XMVector3Length(XMLoadFloat3(&frustumCorners[i]) - center));
		radius = std::max(radius, dist);
	}

	const float lightDistance = radius + RenderConfig::Shadows::LightPadding;
	const XMVECTOR lightPosition = center - lightForward * lightDistance;
	const XMMATRIX lightView = XMMatrixLookAtLH(lightPosition, center, lightUp);

	float minZ = FLT_MAX;
	float maxZ = -FLT_MAX;
	for (std::size_t i = 0; i < frustumCorners.size(); ++i)
	{
		const float z = XMVectorGetZ(XMVector3TransformCoord(XMLoadFloat3(&frustumCorners[i]), lightView));
		minZ = std::min(minZ, z);
		maxZ = std::max(maxZ, z);
	}

	const float nearZ = std::max(0.1f, minZ - RenderConfig::Shadows::LightPadding);
	const float farZ = maxZ + RenderConfig::Shadows::LightPadding;
	XMMATRIX lightProjection = DepthConvention::CreateOrthographicOffCenterLH(
	    -radius, radius, -radius, radius, nearZ, farZ);

	const float shadowMapSize = static_cast<float>(RenderConfig::Shadows::ShadowMapResolution);
	XMMATRIX lightViewProjection = XMMatrixMultiply(lightView, lightProjection);
	XMVECTOR shadowOrigin = XMVector4Transform(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), lightViewProjection);
	shadowOrigin = XMVectorScale(shadowOrigin, shadowMapSize * 0.5f);
	const XMVECTOR roundedOrigin = XMVectorRound(shadowOrigin);
	XMVECTOR offset = XMVectorSubtract(roundedOrigin, shadowOrigin);
	offset = XMVectorScale(offset, 2.0f / shadowMapSize);
	offset = XMVectorSetZ(XMVectorSetW(offset, 0.0f), 0.0f);
	lightProjection.r[3] = XMVectorAdd(lightProjection.r[3], offset);
	lightViewProjection = XMMatrixMultiply(lightView, lightProjection);

	PerViewCameraConstantBufferData cameraData{};
	XMStoreFloat4x4(&cameraData.ViewMTX, lightView);
	XMStoreFloat4x4(&cameraData.ProjectionMTX, lightProjection);
	XMStoreFloat4x4(&cameraData.ViewProjMTX, lightViewProjection);
	XMStoreFloat3(&cameraData.Position, lightPosition);
	XMStoreFloat3(&cameraData.Direction, lightForward);
	cameraData.NearZ = nearZ;
	cameraData.FarZ = farZ;
	return cameraData;
}

XMVECTOR ShadowBuilder::SelectUpVector(FXMVECTOR forward) noexcept
{
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	if (std::abs(XMVectorGetX(XMVector3Dot(forward, up))) > 0.95f)
	{
		up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	return up;
}