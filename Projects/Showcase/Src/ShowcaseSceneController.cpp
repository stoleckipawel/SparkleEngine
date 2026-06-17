#include "ShowcaseSceneController.h"

#include "Scene/GameScene.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
	constexpr float kSceneAssetOscillationSpeedRadiansPerSecond = 0.7f;
	constexpr float kSceneAssetOscillationHalfDistance = 5.25f;

	struct PtlasDemoLane final
	{
		DirectX::XMFLOAT3 Center;
		DirectX::XMFLOAT3 Axis;
		float HalfDistance;
		float SpeedRadiansPerSecond;
		float PhaseOffset;
		float FacingYawRadians;
	};

	constexpr std::array<PtlasDemoLane, 16> kPtlasDemoLanes{{
	    {{-5.2f, 0.0f, -2.3f}, {1.0f, 0.0f, 0.0f}, 1.35f, 0.72f, 0.00f, DirectX::XM_PIDIV2},
	    {{-2.8f, 0.0f, -1.5f}, {1.0f, 0.0f, 0.0f}, 1.15f, 0.89f, 1.10f, DirectX::XM_PIDIV2},
	    {{-0.5f, 0.0f, -2.1f}, {1.0f, 0.0f, 0.0f}, 1.45f, 0.67f, 2.20f, DirectX::XM_PIDIV2},
	    {{2.1f, 0.0f, -1.4f}, {1.0f, 0.0f, 0.0f}, 1.20f, 0.81f, 3.00f, DirectX::XM_PIDIV2},
	    {{4.7f, 0.0f, -2.2f}, {1.0f, 0.0f, 0.0f}, 1.30f, 0.76f, 4.10f, DirectX::XM_PIDIV2},
	    {{-4.6f, 0.0f, 1.7f}, {0.0f, 0.0f, 1.0f}, 1.10f, 0.92f, 0.55f, 0.0f},
	    {{-1.9f, 0.0f, 2.4f}, {0.0f, 0.0f, 1.0f}, 1.25f, 0.70f, 1.70f, 0.0f},
	    {{1.2f, 0.0f, 1.8f}, {0.0f, 0.0f, 1.0f}, 1.35f, 0.84f, 2.80f, 0.0f},
	    {{4.1f, 0.0f, 2.5f}, {0.0f, 0.0f, 1.0f}, 1.15f, 0.73f, 3.65f, 0.0f},
	    {{-5.0f, 1.55f, 0.2f}, {1.0f, 0.0f, 0.0f}, 0.95f, 0.78f, 0.35f, DirectX::XM_PIDIV2},
	    {{-2.3f, 1.55f, 0.9f}, {1.0f, 0.0f, 0.0f}, 1.05f, 0.86f, 1.45f, DirectX::XM_PIDIV2},
	    {{2.8f, 1.55f, -0.1f}, {1.0f, 0.0f, 0.0f}, 1.00f, 0.69f, 2.55f, DirectX::XM_PIDIV2},
	    {{5.1f, 1.55f, 0.7f}, {1.0f, 0.0f, 0.0f}, 0.90f, 0.95f, 3.80f, DirectX::XM_PIDIV2},
	    {{-3.8f, 3.1f, -0.7f}, {0.0f, 0.0f, 1.0f}, 0.75f, 0.74f, 0.90f, 0.0f},
	    {{0.3f, 3.1f, 0.4f}, {0.0f, 0.0f, 1.0f}, 0.85f, 0.82f, 2.05f, 0.0f},
	    {{3.7f, 3.1f, -0.4f}, {0.0f, 0.0f, 1.0f}, 0.70f, 0.88f, 3.25f, 0.0f},
	}};

	DirectX::XMMATRIX BuildCesiumManTransform(
	    const DirectX::XMFLOAT3& translation,
	    float facingYawRadians) noexcept
	{
		const DirectX::XMMATRIX uprightBasisCorrection = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
		const DirectX::XMMATRIX stableFacing = DirectX::XMMatrixRotationY(facingYawRadians);
		return uprightBasisCorrection *
		       stableFacing *
		       DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	}

	DirectX::XMMATRIX BuildOscillatingSceneAssetTransform(float phase) noexcept
	{
		const float aisleX = std::sin(phase) * kSceneAssetOscillationHalfDistance;
		return BuildCesiumManTransform({aisleX, 0.0f, 0.0f}, DirectX::XM_PIDIV2);
	}

	DirectX::XMMATRIX BuildPtlasDemoLaneTransform(const PtlasDemoLane& lane, float timeSeconds) noexcept
	{
		const float phase = timeSeconds * lane.SpeedRadiansPerSecond + lane.PhaseOffset;
		const float signedDistance = std::sin(phase) * lane.HalfDistance;
		const float directionSign = std::cos(phase) >= 0.0f ? 1.0f : -1.0f;
		const DirectX::XMFLOAT3 position{
		    lane.Center.x + lane.Axis.x * signedDistance,
		    lane.Center.y + lane.Axis.y * signedDistance,
		    lane.Center.z + lane.Axis.z * signedDistance};
		const float facingYaw = lane.FacingYawRadians + (directionSign < 0.0f ? DirectX::XM_PI : 0.0f);
		return BuildCesiumManTransform(position, facingYaw);
	}
}

void ShowcaseSceneController::OnSceneReset(GameScene& scene)
{
	Reset();
}

void ShowcaseSceneController::OnSceneAssetsAppended(GameScene& scene)
{
	m_needsTargetRefresh = true;
}

void ShowcaseSceneController::Update(GameScene& scene, const GameSceneUpdateContext& context)
{
	if (context.phase != GameSceneUpdatePhase::PreAnimation)
	{
		return;
	}

	if (m_needsTargetRefresh)
	{
		RefreshAnimatedMeshes(scene);
	}
	ApplyMovement(scene, context.deltaSeconds);
}

void ShowcaseSceneController::Reset() noexcept
{
	m_needsTargetRefresh = true;
	m_motionTimeSeconds = 0.0f;
	m_animatedMeshes.clear();
}

void ShowcaseSceneController::RefreshAnimatedMeshes(GameScene& scene)
{
	SceneMeshes& meshes = scene.GetMeshes();
	m_animatedMeshes.clear();
	for (std::size_t meshIndex = 0; meshIndex < meshes.GetMeshCount(); ++meshIndex)
	{
		const MeshComponent* mesh = meshes.GetMeshComponent(meshIndex);
		if (mesh == nullptr || !mesh->IsSkeletalMeshComponent())
		{
			continue;
		}

		m_animatedMeshes.push_back(
		    AnimatedMesh{
		        .MeshIndex = meshIndex,
		        .BaseTransform = mesh->GetTransform(),
		        .LaneIndex = m_animatedMeshes.size()});
	}

	m_needsTargetRefresh = false;
}

void ShowcaseSceneController::ApplyMovement(GameScene& scene, float deltaSeconds) noexcept
{
	if (m_animatedMeshes.empty())
	{
		return;
	}

	m_motionTimeSeconds += (std::max)(0.0f, deltaSeconds);
	const bool usePtlasDemoLanes = m_animatedMeshes.size() > 1;

	SceneMeshes& meshes = scene.GetMeshes();
	for (const AnimatedMesh& animatedMesh : m_animatedMeshes)
	{
		MeshComponent* mesh = meshes.GetMeshComponent(animatedMesh.MeshIndex);
		if (mesh == nullptr)
		{
			continue;
		}

		const DirectX::XMMATRIX motionTransform =
		    usePtlasDemoLanes
		        ? BuildPtlasDemoLaneTransform(
		              kPtlasDemoLanes[animatedMesh.LaneIndex % kPtlasDemoLanes.size()],
		              m_motionTimeSeconds)
		        : BuildOscillatingSceneAssetTransform(
		              m_motionTimeSeconds * kSceneAssetOscillationSpeedRadiansPerSecond);
		mesh->SetTransform(Transform(animatedMesh.BaseTransform.GetWorldMatrix() * motionTransform));
	}
}
