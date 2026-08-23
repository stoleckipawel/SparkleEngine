#include "PCH.h"

#include "View/RenderViewBuilder.h"

#include "Config/DepthConvention.h"
#include "Core/Public/Math/WorldCoordinateSystem.h"
#include "View/RenderView.h"
#include "View/RenderViewState.h"

#include <algorithm>

using namespace DirectX;

RhiViewport RenderViewBuilder::BuildViewport(RenderViewportExtent extent) noexcept
{
	return RhiViewport{
	    .X = 0.0f,
	    .Y = 0.0f,
	    .Width = static_cast<float>(extent.Width),
	    .Height = static_cast<float>(extent.Height),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f};
}

RhiRect RenderViewBuilder::BuildScissorRect(RenderViewportExtent extent) noexcept
{
	return RhiRect{
	    .Left = 0,
	    .Top = 0,
	    .Right = static_cast<std::int32_t>(extent.Width),
	    .Bottom = static_cast<std::int32_t>(extent.Height)};
}

void RenderViewBuilder::Build(RenderView& output, RenderViewState& state, const RenderViewBuildRequest& request) const noexcept
{
	output.ResetForReuse();
	output.viewportId = request.ViewportRequest.ViewportId;
	output.selection = request.ViewportRequest.ViewSelection;
	output.kind = request.ViewportRequest.ViewKind;
	output.renderExtent = request.RenderExtent;
	output.outputExtent = request.OutputExtent;
	output.displaySettings = ResolvedViewportDisplaySettings::Resolve(request.ViewportRequest.Exposure);
	output.camera = request.Input.Camera;

	const RenderViewportExtent projectionExtent =
	    request.ViewportRequest.Extent.IsValid() ? request.ViewportRequest.Extent : request.OutputExtent;
	const float projectionWidth = static_cast<float>((std::max) (projectionExtent.Width, 1u));
	const float projectionHeight = static_cast<float>((std::max) (projectionExtent.Height, 1u));
	output.camera.AspectRatio = projectionWidth / projectionHeight;

	const XMVECTOR position = XMLoadFloat3(&output.camera.Position);
	const XMVECTOR direction = XMLoadFloat3(&output.camera.Direction);
	const XMVECTOR target = XMVectorAdd(position, direction);
	const XMVECTOR worldUp = XMVectorSet(WorldCoordinates::kUpX, WorldCoordinates::kUpY, WorldCoordinates::kUpZ, 0.0f);
	const XMMATRIX worldToView = XMMatrixLookAtLH(position, target, worldUp);

	const float nearZ = output.camera.NearZ;
	const float farZ = output.camera.FarZ;
	XMMATRIX viewToClip;
	if (output.camera.ProjectionKind == CameraProjectionKind::Orthographic)
	{
		const float height = (std::max) (output.camera.OrthographicHeightMeters, 0.001f);
		const float width = height * (std::max) (output.camera.AspectRatio, 0.001f);
		viewToClip =
		    DepthConvention::CreateOrthographicOffCenterLH(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, nearZ, farZ);
	}
	else
	{
		viewToClip =
		    DepthConvention::CreatePerspectiveFovLH(XMConvertToRadians(output.camera.FovYDegrees), output.camera.AspectRatio, nearZ, farZ);
	}

	const XMMATRIX worldToClip = XMMatrixMultiply(worldToView, viewToClip);
	XMStoreFloat4x4(&output.cameraUniform.ViewMTX, worldToView);
	XMStoreFloat4x4(&output.cameraUniform.ProjectionMTX, viewToClip);
	XMStoreFloat4x4(&output.cameraUniform.ViewProjMTX, worldToClip);
	XMStoreFloat4x4(&output.cameraUniform.InvViewMTX, XMMatrixInverse(nullptr, worldToView));
	XMStoreFloat4x4(&output.cameraUniform.InvProjectionMTX, XMMatrixInverse(nullptr, viewToClip));
	output.cameraUniform.Position = output.camera.Position;
	output.cameraUniform.NearZ = nearZ;
	output.cameraUniform.FarZ = farZ;
	output.cameraUniform.Direction = output.camera.Direction;
	output.frustum.ExtractFromViewProjection(output.cameraUniform.ViewProjMTX);

	output.viewport = BuildViewport(request.RenderExtent);
	output.scissorRect = BuildScissorRect(request.RenderExtent);
	const float renderWidth = static_cast<float>((std::max) (request.RenderExtent.Width, 1u));
	const float renderHeight = static_cast<float>((std::max) (request.RenderExtent.Height, 1u));
	output.uniform.ViewportSize = {renderWidth, renderHeight};
	output.uniform.ViewportSizeInv = {1.0f / renderWidth, 1.0f / renderHeight};
	output.uniform.ViewModeIndex = static_cast<std::uint32_t>(request.ViewMode);

	output.temporalUniform = state.BuildTemporal(
	    RenderViewStateBuildInput{
	        .ViewInput = request.Input,
	        .Identity =
	            {
	                .ViewportId = output.viewportId,
	                .Selection = output.selection.Value,
	                .Kind = output.kind,
	            },
	        .Camera = output.cameraUniform,
	        .RenderExtent = request.RenderExtent,
	        .FrameId = request.FrameId,
	        .SceneGeneration = request.SceneGeneration,
	        .ShaderGeneration = request.ShaderGeneration,
	        .ImageProviderGeneration = request.ImageProviderGeneration,
	        .GraphTopologyGeneration = request.GraphTopologyGeneration});
}
