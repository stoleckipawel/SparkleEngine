#pragma once

#include "GameFramework/Public/Rendering/RenderViewInput.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"

#include <DirectXMath.h>

#include <cstdint>

enum class RenderViewInvalidationReason : std::uint32_t
{
	None = 0u,
	ViewIdentity = 1u << 0u,
	SceneGeneration = 1u << 1u,
	CameraCut = 1u << 2u,
	CameraTeleport = 1u << 3u,
	CameraDiscontinuity = 1u << 4u,
	GraphTopology = 1u << 5u,
	ShaderGeneration = 1u << 6u,
	ImageProviderGeneration = 1u << 7u,
	ExplicitReset = 1u << 8u,
	ProjectionChange = 1u << 9u,
};

struct RenderViewStateIdentity final
{
	std::uint64_t ViewportId = 0u;
	std::uint64_t Selection = 0u;
	RenderViewKind Kind = RenderViewKind::Game;

	bool operator==(const RenderViewStateIdentity&) const noexcept = default;
};

struct RenderViewStateBuildInput final
{
	const RenderViewInput& ViewInput;
	RenderViewStateIdentity Identity = {};
	ViewCameraUniformData Camera = {};
	RenderViewportExtent RenderExtent = {};
	std::uint64_t FrameId = 0u;
	std::uint64_t SceneGeneration = 0u;
	std::uint64_t ShaderGeneration = 0u;
	std::uint64_t ImageProviderGeneration = 0u;
	std::uint64_t GraphTopologyGeneration = 0u;
};

class RenderViewState final
{
public:
	void Invalidate(RenderViewInvalidationReason reason) noexcept;
	ViewTemporalUniformData BuildTemporal(const RenderViewStateBuildInput& input) noexcept;
	RenderViewInvalidationReason GetLastInvalidationReasons() const noexcept { return m_lastInvalidationReasons; }

private:
	struct CameraPose final
	{
		DirectX::XMFLOAT4X4 WorldToViewMatrix = {};
		DirectX::XMFLOAT4X4 ViewToClipMatrix = {};
		DirectX::XMFLOAT4X4 WorldToClipMatrix = {};
		DirectX::XMFLOAT3 Position = {};
		DirectX::XMFLOAT3 Direction = {0.0f, 0.0f, 1.0f};
		CameraProjectionKind ProjectionKind = CameraProjectionKind::Perspective;
		float FovYDegrees = 60.0f;
		float NearZ = 0.0f;
		float FarZ = 0.0f;
		float OrthographicHeightMeters = 0.0f;
	};

	void ObserveIdentityAndGenerations(const RenderViewStateBuildInput& input) noexcept;
	static RenderViewInvalidationReason CombineInvalidationReasons(
	    RenderViewInvalidationReason left,
	    RenderViewInvalidationReason right) noexcept;
	static CameraPose CapturePose(const RenderViewStateBuildInput& input) noexcept;
	static bool HasProjectionChange(const CameraPose& previousPose, const CameraPose& currentPose) noexcept;
	static bool IsLikelyCameraCut(const CameraPose& previousPose, const CameraPose& currentPose) noexcept;

	RenderViewStateIdentity m_identity = {};
	CameraPose m_previousPose = {};
	DirectX::XMFLOAT2 m_previousJitterNdc = {};
	std::uint64_t m_sceneGeneration = 0u;
	std::uint64_t m_shaderGeneration = 0u;
	std::uint64_t m_imageProviderGeneration = 0u;
	std::uint64_t m_graphTopologyGeneration = 0u;
	std::uint32_t m_temporalSampleIndex = 0u;
	RenderViewInvalidationReason m_pendingInvalidationReasons = RenderViewInvalidationReason::ExplicitReset;
	RenderViewInvalidationReason m_lastInvalidationReasons = RenderViewInvalidationReason::ExplicitReset;
	bool m_hasIdentity = false;
	bool m_hasPreviousPose = false;
};
