#include "ShowcaseSceneController.h"

#include "Scene/GameScene.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kSceneAssetOscillationSpeedRadiansPerSecond = 0.7f;
	constexpr float kSceneAssetOscillationHalfDistance = 5.25f;

	DirectX::XMMATRIX BuildOscillatingSceneAssetTransform(float phase) noexcept
	{
		const float aisleX = std::sin(phase) * kSceneAssetOscillationHalfDistance;
		const DirectX::XMMATRIX uprightBasisCorrection = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
		const DirectX::XMMATRIX stableFacing = DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2);
		return uprightBasisCorrection *
		       stableFacing *
		       DirectX::XMMatrixTranslation(aisleX, 0.0f, 0.0f);
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
		        .BaseTransform = mesh->GetTransform()});
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
	const DirectX::XMMATRIX motionTransform =
	    BuildOscillatingSceneAssetTransform(m_motionTimeSeconds * kSceneAssetOscillationSpeedRadiansPerSecond);

	SceneMeshes& meshes = scene.GetMeshes();
	for (const AnimatedMesh& animatedMesh : m_animatedMeshes)
	{
		MeshComponent* mesh = meshes.GetMeshComponent(animatedMesh.MeshIndex);
		if (mesh == nullptr)
		{
			continue;
		}

		mesh->SetTransform(Transform(animatedMesh.BaseTransform.GetWorldMatrix() * motionTransform));
	}
}
