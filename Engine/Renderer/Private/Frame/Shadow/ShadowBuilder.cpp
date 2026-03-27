#include "PCH.h"

#include "Frame/Shadow/ShadowBuilder.h"

#include "Renderer/Public/GPU/DepthConvention.h"
#include "RHI/Public/RenderConfig.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

ShadowBuildResult ShadowBuilder::Build(
	const CameraSnapshot& mainCamera,
	const XMFLOAT3& lightDirection,
	float cascadeNearZ,
	float cascadeFarZ) const noexcept
{
	ShadowBuildResult result{};
	result.shadow.ShadowMapSize = static_cast<float>(RenderConfig::Shadows::ShadowMapResolution);
	result.shadow.DepthBias = RenderConfig::Shadows::DepthBias;
	result.shadow.NormalBias = RenderConfig::Shadows::NormalBias;
	result.shadow.CascadeFarDepth = cascadeFarZ;
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
	const float nearZ = std::clamp(cascadeNearZ, mainCamera.nearZ, shadowDistance);
	const float farZ = std::clamp(cascadeFarZ, nearZ + 0.001f, shadowDistance);
	const float cascadeRadius = farZ;
	(void)nearZ;
	result.cameraData = BuildLightCameraForSphere(mainCamera.position, cascadeRadius, lightDirection);
	result.shadow.ViewProjMTX = result.cameraData.ViewProjMTX;

	return result;
}

PerViewCameraConstantBufferData ShadowBuilder::BuildLightCameraForSphere(
	const XMFLOAT3& sphereCenter,
	float sphereRadius,
	const XMFLOAT3& lightDirection) noexcept
{
	const XMVECTOR center = XMLoadFloat3(&sphereCenter);

	XMVECTOR lightForward = XMVector3Normalize(XMLoadFloat3(&lightDirection));
	if (XMVectorGetX(XMVector3LengthSq(lightForward)) < 1.0e-6f)
	{
		lightForward = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	}

	const XMVECTOR lightUp = SelectUpVector(lightForward);
	const float lightPadding = std::max(10.0f, sphereRadius * 0.15f);
	const float lightDistance = sphereRadius + lightPadding;
	const XMVECTOR lightPosition = center - lightForward * lightDistance;
	const XMMATRIX lightView = XMMatrixLookAtLH(lightPosition, center, lightUp);

	const float nearZ = std::max(0.1f, lightPadding);
	const float farZ = (sphereRadius * 2.0f) + lightPadding;
	XMMATRIX lightProjection = DepthConvention::CreateOrthographicOffCenterLH(
	    -sphereRadius,
	    sphereRadius,
	    -sphereRadius,
	    sphereRadius,
	    nearZ,
	    farZ);

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