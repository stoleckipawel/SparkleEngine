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

	DirectX::XMMATRIX BuildCesiumManSponzaPatrolTransform(float phase) noexcept
	{
		const float aisleX = std::sin(phase) * kSponzaPtlasPatrolHalfDistance;
		const DirectX::XMMATRIX cesiumManUprightBasisCorrection = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
		const DirectX::XMMATRIX stableFacing = DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2);
		return cesiumManUprightBasisCorrection *
		       stableFacing *
		       DirectX::XMMatrixTranslation(aisleX, 0.0f, 0.0f);
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
	const DirectX::XMMATRIX patrolTransform =
	    BuildCesiumManSponzaPatrolTransform(m_sponzaPatrolTimeSeconds * kSponzaPtlasPatrolSpeedRadiansPerSecond);

	SceneMeshes& meshes = scene.GetMeshes();
	for (const AnimatedMesh& animatedMesh : m_sponzaAnimatedMeshes)
	{
		MeshComponent* mesh = meshes.GetMeshComponent(animatedMesh.MeshIndex);
		if (mesh == nullptr)
		{
			continue;
		}

		mesh->SetTransform(Transform(animatedMesh.BaseTransform.GetWorldMatrix() * patrolTransform));
	}
}
