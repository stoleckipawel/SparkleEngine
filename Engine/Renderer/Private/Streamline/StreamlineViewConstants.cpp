#include "../PCH.h"
#include "Streamline/StreamlineViewConstants.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	#include <cmath>
	#include <cstddef>
	#include <string>
	#include <string_view>

static const auto g_streamlineViewConstantsLogger = Logging::GetOrCreateLogger("Renderer.StreamlineViewConstants");

class StreamlineViewConstantTranslation final
{
  public:
	static void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			Diagnostics::Fatal(g_streamlineViewConstantsLogger, __FILE__, __LINE__, std::string(message));
		}
	}

	static void FillIdentity(sl::float4x4& matrix) noexcept
	{
		matrix = {};
		matrix[0] = sl::float4{1.0f, 0.0f, 0.0f, 0.0f};
		matrix[1] = sl::float4{0.0f, 1.0f, 0.0f, 0.0f};
		matrix[2] = sl::float4{0.0f, 0.0f, 1.0f, 0.0f};
		matrix[3] = sl::float4{0.0f, 0.0f, 0.0f, 1.0f};
	}

	static sl::float4x4 ToStreamlineMatrixFromMatrix(DirectX::FXMMATRIX source)
	{
		DirectX::XMFLOAT4X4 stored{};
		DirectX::XMStoreFloat4x4(&stored, source);
		return ToStreamlineMatrix(stored);
	}

	static sl::float3 ToStreamlineFloat3(const DirectX::XMFLOAT3& source)
	{
		return sl::float3{source.x, source.y, source.z};
	}

	static sl::float3 NormalizeToStreamlineFloat3(const DirectX::XMFLOAT3& source)
	{
		const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&source);
		const float lengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector));
		Require(lengthSq > 1.0e-8f, "Streamline view constants contain a zero-length direction.");

		DirectX::XMFLOAT3 normalized{};
		DirectX::XMStoreFloat3(&normalized, DirectX::XMVector3Normalize(vector));
		return ToStreamlineFloat3(normalized);
	}

	static sl::float3 MatrixBasisRowToStreamlineFloat3(const DirectX::XMFLOAT4X4& matrix, std::size_t row)
	{
		const DirectX::XMFLOAT3 basis{matrix.m[row][0], matrix.m[row][1], matrix.m[row][2]};
		return NormalizeToStreamlineFloat3(basis);
	}

	static float CalculateVerticalFovRadians(const DirectX::XMFLOAT4X4& projection)
	{
		const float projectionYScale = projection.m[1][1];
		Require(projectionYScale > 1.0e-6f, "Streamline camera projection has a non-positive vertical scale.");
		return 2.0f * std::atan(1.0f / projectionYScale);
	}

	static float CalculateAspectRatio(const DirectX::XMFLOAT4X4& projection)
	{
		const float projectionXScale = projection.m[0][0];
		const float projectionYScale = projection.m[1][1];
		Require(
		    projectionXScale > 1.0e-6f && projectionYScale > 1.0e-6f,
		    "Streamline camera projection has a non-positive aspect scale.");
		return projectionYScale / projectionXScale;
	}

	static sl::float2 ConvertNdcJitterToPixelJitter(const DirectX::XMFLOAT2& jitterNdc, const RenderViewportExtent& renderExtent)
	{
		return sl::float2{
		    jitterNdc.x * static_cast<float>(renderExtent.Width) * 0.5f,
		    -jitterNdc.y * static_cast<float>(renderExtent.Height) * 0.5f};
	}

	static sl::float2 BuildMotionVectorScale(const StreamlineViewConstantsInput& input)
	{
		Require(
		    input.RenderExtent.Width > 0 && input.RenderExtent.Height > 0,
		    "Streamline motion-vector scale requires a non-empty render extent.");
		const float directionScale = input.MotionVectorsCurrentMinusPrevious ? 1.0f : -1.0f;
		return sl::float2{
		    directionScale / static_cast<float>(input.RenderExtent.Width),
		    directionScale / static_cast<float>(input.RenderExtent.Height)};
	}
};

sl::float4x4 ToStreamlineMatrix(const DirectX::XMFLOAT4X4& source)
{
	sl::float4x4 matrix{};
	matrix[0] = sl::float4{source.m[0][0], source.m[0][1], source.m[0][2], source.m[0][3]};
	matrix[1] = sl::float4{source.m[1][0], source.m[1][1], source.m[1][2], source.m[1][3]};
	matrix[2] = sl::float4{source.m[2][0], source.m[2][1], source.m[2][2], source.m[2][3]};
	matrix[3] = sl::float4{source.m[3][0], source.m[3][1], source.m[3][2], source.m[3][3]};
	return matrix;
}

void FillStreamlineViewConstants(sl::Constants& constants, const StreamlineViewConstantsInput& input)
{
	StreamlineViewConstantTranslation::Require(
	    input.RenderExtent.Width > 0 && input.RenderExtent.Height > 0,
	    "Streamline view constants require a non-empty render extent.");
	StreamlineViewConstantTranslation::Require(
	    input.Camera.NearZ > 0.0f && input.Camera.FarZ > input.Camera.NearZ,
	    "Streamline view constants contain an invalid camera clip range.");

	DirectX::XMMATRIX clipToPrevClip = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX prevClipToClip = DirectX::XMMatrixIdentity();
	if (input.TemporalState.HistoryValid)
	{
		const DirectX::XMMATRIX invProjection = DirectX::XMLoadFloat4x4(&input.Camera.InvProjectionMTX);
		const DirectX::XMMATRIX invView = DirectX::XMLoadFloat4x4(&input.Camera.InvViewMTX);
		const DirectX::XMMATRIX previousWorldToClip =
		    DirectX::XMLoadFloat4x4(&input.TemporalData.PreviousWorldToClipMatrix);
		clipToPrevClip = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(invProjection, invView), previousWorldToClip);
		prevClipToClip = DirectX::XMMatrixInverse(nullptr, clipToPrevClip);
	}

	constants.cameraViewToClip = ToStreamlineMatrix(input.Camera.ProjectionMTX);
	constants.clipToCameraView = ToStreamlineMatrix(input.Camera.InvProjectionMTX);
	StreamlineViewConstantTranslation::FillIdentity(constants.clipToLensClip);
	constants.clipToPrevClip = StreamlineViewConstantTranslation::ToStreamlineMatrixFromMatrix(clipToPrevClip);
	constants.prevClipToClip = StreamlineViewConstantTranslation::ToStreamlineMatrixFromMatrix(prevClipToClip);
	constants.jitterOffset =
	    StreamlineViewConstantTranslation::ConvertNdcJitterToPixelJitter(input.TemporalState.CurrentJitterNdc, input.RenderExtent);
	constants.mvecScale = StreamlineViewConstantTranslation::BuildMotionVectorScale(input);
	constants.cameraPinholeOffset = sl::float2{0.0f, 0.0f};
	constants.cameraPos = StreamlineViewConstantTranslation::ToStreamlineFloat3(input.Camera.Position);
	constants.cameraUp = StreamlineViewConstantTranslation::MatrixBasisRowToStreamlineFloat3(input.Camera.InvViewMTX, 1u);
	constants.cameraRight = StreamlineViewConstantTranslation::MatrixBasisRowToStreamlineFloat3(input.Camera.InvViewMTX, 0u);
	constants.cameraFwd = StreamlineViewConstantTranslation::NormalizeToStreamlineFloat3(input.Camera.Direction);
	constants.cameraNear = input.Camera.NearZ;
	constants.cameraFar = input.Camera.FarZ;
	constants.cameraFOV = StreamlineViewConstantTranslation::CalculateVerticalFovRadians(input.Camera.ProjectionMTX);
	constants.cameraAspectRatio = StreamlineViewConstantTranslation::CalculateAspectRatio(input.Camera.ProjectionMTX);
	constants.depthInverted = input.ReversedDeviceDepth ? sl::Boolean::eTrue : sl::Boolean::eFalse;
	constants.cameraMotionIncluded = sl::Boolean::eTrue;
	constants.motionVectors3D = sl::Boolean::eFalse;
	constants.reset = input.ResetRequested || !input.TemporalState.HistoryValid ? sl::Boolean::eTrue : sl::Boolean::eFalse;
	constants.motionVectorsJittered = sl::Boolean::eFalse;
}
#endif
