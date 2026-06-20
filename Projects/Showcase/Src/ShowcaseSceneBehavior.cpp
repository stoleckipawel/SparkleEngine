#include "ShowcaseSceneBehavior.h"

#include "Scene/GameScene.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>

namespace
{
	constexpr const char* kSponzaLevelName = "Sponza";
	constexpr float kSponzaPtlasPatrolSpeedRadiansPerSecond = 0.7f;
	constexpr float kSponzaPtlasPatrolHalfDistance = 5.25f;
	constexpr float kSponzaPtlasCrossPatrolHalfDistance = 3.25f;
	constexpr float kSponzaPtlasActorSpacing = 1.75f;
	constexpr float kSponzaPtlasActorPhaseStep = 1.35f;

	enum class PatrolAxis
	{
		X,
		Z
	};

	DirectX::XMMATRIX BuildCesiumManSponzaPatrolTransform(float phase, PatrolAxis axis, float laneOffset) noexcept
	{
		const float wave = std::sin(phase);
		const float velocitySign = std::cos(phase) >= 0.0f ? 1.0f : -1.0f;
		const bool xAxis = axis == PatrolAxis::X;
		const float aisleX = xAxis ? wave * kSponzaPtlasPatrolHalfDistance : laneOffset;
		const float aisleZ = xAxis ? laneOffset : wave * kSponzaPtlasCrossPatrolHalfDistance;
		const float facingYaw =
		    xAxis
		        ? (velocitySign >= 0.0f ? 0.0f : DirectX::XM_PI)
		        : (velocitySign >= 0.0f ? DirectX::XM_PIDIV2 : -DirectX::XM_PIDIV2);
		const DirectX::XMMATRIX cesiumManUprightBasisCorrection = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
		return cesiumManUprightBasisCorrection *
		       DirectX::XMMatrixRotationY(facingYaw) *
		       DirectX::XMMatrixTranslation(aisleX, 0.0f, aisleZ);
	}
}

void ShowcaseSceneBehavior::Update(GameScene& scene, float deltaSeconds) noexcept
{
	if (scene.GetActiveLevelName() != kSponzaLevelName)
	{
		Reset();
		return;
	}

	CaptureSponzaAnimatedMeshes(scene);
	UpdateSponzaPtlasPatrol(scene, deltaSeconds);
}

void ShowcaseSceneBehavior::Reset() noexcept
{
	m_sponzaPatrolTimeSeconds = 0.0f;
	m_lastSponzaMeshCount = 0;
	m_sponzaAnimatedMeshes.clear();
}

void ShowcaseSceneBehavior::CaptureSponzaAnimatedMeshes(GameScene& scene)
{
	SceneMeshes& meshes = scene.GetMeshes();
	const std::size_t meshCount = meshes.GetMeshCount();
	if (meshCount == m_lastSponzaMeshCount && !m_sponzaAnimatedMeshes.empty())
	{
		return;
	}

	m_lastSponzaMeshCount = meshCount;
	m_sponzaAnimatedMeshes.clear();
	for (std::size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
	{
		const MeshComponent* mesh = meshes.GetMeshComponent(meshIndex);
		if (mesh == nullptr || !mesh->IsSkeletalMeshComponent())
		{
			continue;
		}

		m_sponzaAnimatedMeshes.push_back(
		    AnimatedMesh{
		        .MeshIndex = meshIndex,
		        .BaseTransform = mesh->GetTransform()});
	}
}

void ShowcaseSceneBehavior::UpdateSponzaPtlasPatrol(GameScene& scene, float deltaSeconds) noexcept
{
	if (m_sponzaAnimatedMeshes.empty())
	{
		return;
	}

	m_sponzaPatrolTimeSeconds += (std::max)(0.0f, deltaSeconds);
	SceneMeshes& meshes = scene.GetMeshes();
	const std::size_t xPatrolCount = (m_sponzaAnimatedMeshes.size() + 1u) / 2u;
	const std::size_t zPatrolCount = m_sponzaAnimatedMeshes.size() - xPatrolCount;
	for (std::size_t actorIndex = 0; actorIndex < m_sponzaAnimatedMeshes.size(); ++actorIndex)
	{
		const AnimatedMesh& animatedMesh = m_sponzaAnimatedMeshes[actorIndex];
		MeshComponent* mesh = meshes.GetMeshComponent(animatedMesh.MeshIndex);
		if (mesh == nullptr)
		{
			continue;
		}

		const bool xPatrol = actorIndex < xPatrolCount;
		const std::size_t routeIndex = xPatrol ? actorIndex : actorIndex - xPatrolCount;
		const std::size_t routeCount = xPatrol ? xPatrolCount : zPatrolCount;
		const float routeCenter = (static_cast<float>(routeCount) - 1.0f) * 0.5f;
		const float actorOffset = static_cast<float>(routeIndex) - routeCenter;
		const float phase =
		    m_sponzaPatrolTimeSeconds * kSponzaPtlasPatrolSpeedRadiansPerSecond +
		    static_cast<float>(actorIndex) * kSponzaPtlasActorPhaseStep;
		const DirectX::XMMATRIX patrolTransform =
		    BuildCesiumManSponzaPatrolTransform(
		        phase,
		        xPatrol ? PatrolAxis::X : PatrolAxis::Z,
		        actorOffset * kSponzaPtlasActorSpacing);
		mesh->SetTransform(Transform(animatedMesh.BaseTransform.GetWorldMatrix() * patrolTransform));
	}
}
