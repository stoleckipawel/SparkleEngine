#include "../PCH.h"
#include "Streamline/StreamlineViewConstants.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <cmath>
#include <cstddef>

namespace
{
	void FillIdentity(sl::float4x4& matrix) noexcept
	{
		matrix = {};
		matrix[0] = sl::float4{1.0f, 0.0f, 0.0f, 0.0f};
		matrix[1] = sl::float4{0.0f, 1.0f, 0.0f, 0.0f};
		matrix[2] = sl::float4{0.0f, 0.0f, 1.0f, 0.0f};
		matrix[3] = sl::float4{0.0f, 0.0f, 0.0f, 1.0f};
	}

	sl::float4x4 ToStreamlineMatrixFromMatrix(DirectX::FXMMATRIX source) noexcept
	{
		DirectX::XMFLOAT4X4 stored{};
		DirectX::XMStoreFloat4x4(&stored, source);
		return ToStreamlineMatrix(stored);
	}

	sl::float3 ToStreamlineFloat3(const DirectX::XMFLOAT3& source) noexcept
	{
		return sl::float3{source.x, source.y, source.z};
	}

	sl::float3 NormalizeToStreamlineFloat3(const DirectX::XMFLOAT3& source, DirectX::FXMVECTOR fallback) noexcept
	{
		const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&source);
		const float lengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector));
		if (lengthSq <= 1.0e-8f)
		{
			DirectX::XMFLOAT3 storedFallback{};
			DirectX::XMStoreFloat3(&storedFallback, fallback);
			return ToStreamlineFloat3(storedFallback);
		}

		DirectX::XMFLOAT3 normalized{};
		DirectX::XMStoreFloat3(&normalized, DirectX::XMVector3Normalize(vector));
		return ToStreamlineFloat3(normalized);
	}

	sl::float3 MatrixBasisRowToStreamlineFloat3(const DirectX::XMFLOAT4X4& matrix, std::size_t row) noexcept
	{
		const DirectX::XMFLOAT3 basis{matrix.m[row][0], matrix.m[row][1], matrix.m[row][2]};
		return NormalizeToStreamlineFloat3(basis, DirectX::XMVectorZero());
	}

	float CalculateVerticalFovRadians(const DirectX::XMFLOAT4X4& projection) noexcept
	{
		const float projectionYScale = projection.m[1][1];
		if (std::abs(projectionYScale) <= 1.0e-6f)
		{
			return 1.04719755f;
		}
		return 2.0f * std::atan(1.0f / projectionYScale);
	}

	float CalculateAspectRatio(const DirectX::XMFLOAT4X4& projection, const RenderViewportExtent& renderExtent) noexcept
	{
		const float projectionXScale = projection.m[0][0];
		const float projectionYScale = projection.m[1][1];
		if (std::abs(projectionXScale) > 1.0e-6f && std::abs(projectionYScale) > 1.0e-6f)
		{
			return projectionYScale / projectionXScale;
		}
		return renderExtent.Height > 0 ? static_cast<float>(renderExtent.Width) / static_cast<float>(renderExtent.Height) : 1.0f;
	}

	sl::float2 ConvertNdcJitterToPixelJitter(
	    const DirectX::XMFLOAT2& jitterNdc,
	    const RenderViewportExtent& renderExtent) noexcept
	{
		return sl::float2{
		    jitterNdc.x * static_cast<float>(renderExtent.Width) * 0.5f,
		    -jitterNdc.y * static_cast<float>(renderExtent.Height) * 0.5f};
	}

	sl::float2 BuildMotionVectorScale(const StreamlineViewConstantsInput& input) noexcept
	{
		const float directionScale = input.MotionVectorsCurrentMinusPrevious ? -1.0f : 1.0f;
		return sl::float2{
		    input.RenderExtent.Width > 0 ? directionScale / static_cast<float>(input.RenderExtent.Width) : directionScale,
		    input.RenderExtent.Height > 0 ? directionScale / static_cast<float>(input.RenderExtent.Height) : directionScale};
	}
}

sl::float4x4 ToStreamlineMatrix(const DirectX::XMFLOAT4X4& source) noexcept
{
	sl::float4x4 matrix{};
	matrix[0] = sl::float4{source.m[0][0], source.m[0][1], source.m[0][2], source.m[0][3]};
	matrix[1] = sl::float4{source.m[1][0], source.m[1][1], source.m[1][2], source.m[1][3]};
	matrix[2] = sl::float4{source.m[2][0], source.m[2][1], source.m[2][2], source.m[2][3]};
	matrix[3] = sl::float4{source.m[3][0], source.m[3][1], source.m[3][2], source.m[3][3]};
	return matrix;
}

void FillStreamlineViewConstants(sl::Constants& constants, const StreamlineViewConstantsInput& input) noexcept
{
	DirectX::XMMATRIX clipToPrevClip = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX prevClipToClip = DirectX::XMMatrixIdentity();
	if (input.TemporalState.HistoryValid)
	{
		const DirectX::XMMATRIX invProjection = DirectX::XMLoadFloat4x4(&input.Camera.InvProjectionMTX);
		const DirectX::XMMATRIX invView = DirectX::XMLoadFloat4x4(&input.Camera.InvViewMTX);
		const DirectX::XMMATRIX previousViewProjection = DirectX::XMLoadFloat4x4(&input.TemporalData.PrevViewProjMTX);
		clipToPrevClip = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(invProjection, invView), previousViewProjection);
		prevClipToClip = DirectX::XMMatrixInverse(nullptr, clipToPrevClip);
	}

	constants.cameraViewToClip = ToStreamlineMatrix(input.Camera.ProjectionMTX);
	constants.clipToCameraView = ToStreamlineMatrix(input.Camera.InvProjectionMTX);
	FillIdentity(constants.clipToLensClip);
	constants.clipToPrevClip = ToStreamlineMatrixFromMatrix(clipToPrevClip);
	constants.prevClipToClip = ToStreamlineMatrixFromMatrix(prevClipToClip);
	constants.jitterOffset = ConvertNdcJitterToPixelJitter(input.TemporalState.JitterCurrent, input.RenderExtent);
	constants.mvecScale = BuildMotionVectorScale(input);
	constants.cameraPinholeOffset = sl::float2{0.0f, 0.0f};
	constants.cameraPos = ToStreamlineFloat3(input.Camera.Position);
	constants.cameraUp = MatrixBasisRowToStreamlineFloat3(input.Camera.InvViewMTX, 1u);
	constants.cameraRight = MatrixBasisRowToStreamlineFloat3(input.Camera.InvViewMTX, 0u);
	constants.cameraFwd = NormalizeToStreamlineFloat3(input.Camera.Direction, DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	constants.cameraNear = input.Camera.NearZ;
	constants.cameraFar = input.Camera.FarZ;
	constants.cameraFOV = CalculateVerticalFovRadians(input.Camera.ProjectionMTX);
	constants.cameraAspectRatio = CalculateAspectRatio(input.Camera.ProjectionMTX, input.RenderExtent);
	constants.depthInverted = input.ReversedDeviceDepth ? sl::Boolean::eTrue : sl::Boolean::eFalse;
	constants.cameraMotionIncluded = sl::Boolean::eTrue;
	constants.motionVectors3D = sl::Boolean::eFalse;
	constants.reset = input.ResetRequested || !input.TemporalState.HistoryValid ? sl::Boolean::eTrue : sl::Boolean::eFalse;
	constants.motionVectorsJittered = sl::Boolean::eFalse;
}
#endif
