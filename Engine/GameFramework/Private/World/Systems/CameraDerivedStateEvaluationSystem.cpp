#include "PCH.h"
#include "World/Systems/CameraDerivedStateEvaluationSystem.h"

#include "Core/Public/Math/WorldCoordinateSystem.h"
#include "World/ECS/Components/TransformComponents.h"

#include <cmath>

namespace ECS
{
	void CameraDerivedStateEvaluationSystem::Evaluate(const LocalTransform& local, CameraDerivedState& derived) noexcept
	{
		const DirectX::XMVECTOR forward = DirectX::XMVector3Rotate(
		    DirectX::XMVectorSet(WorldCoordinates::kForwardX, WorldCoordinates::kForwardY, WorldCoordinates::kForwardZ, 0.0f),
		    DirectX::XMLoadFloat4(&local.Rotation));
		DirectX::XMStoreFloat3(&derived.Direction, DirectX::XMVector3Normalize(forward));
	}
}
