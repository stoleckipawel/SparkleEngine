#include "PCH.h"

#include "World/Systems/OscillatingMeshMotionSystem.h"

#include "Scene/Transform.h"
#include "World/ECS/Components/MotionComponents.h"
#include "World/WorldTransformConversion.h"

#include <array>
#include <cmath>

class OscillatingMeshMotionSystemImplementation final
{
  public:
	struct MotionLane final
	{
		DirectX::XMFLOAT3 Center;
		DirectX::XMFLOAT3 Axis;
		float HalfDistance;
		float Speed;
		float Phase;
		float FacingYaw;
	};

	static constexpr std::array<MotionLane, 16> Lanes{{
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

	static DirectX::XMMATRIX BuildCharacterTransform(const DirectX::XMFLOAT3& translation, float yaw) noexcept
	{
		return DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2) * DirectX::XMMatrixRotationY(yaw) *
		       DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
	}
};

namespace ECS
{
	void OscillatingMeshMotionSystem::Apply(
	    const OscillatingMotion& motion,
	    float timeSeconds,
	    bool useLanes,
	    LocalTransform& transform) noexcept
	{
		DirectX::XMMATRIX animated;
		if (useLanes)
		{
			const OscillatingMeshMotionSystemImplementation::MotionLane& lane = OscillatingMeshMotionSystemImplementation::Lanes[motion.LaneIndex % OscillatingMeshMotionSystemImplementation::Lanes.size()];
			const float phase = timeSeconds * lane.Speed + lane.Phase;
			const float distance = std::sin(phase) * lane.HalfDistance;
			const DirectX::XMFLOAT3 position{
			    lane.Center.x + lane.Axis.x * distance,
			    lane.Center.y + lane.Axis.y * distance,
			    lane.Center.z + lane.Axis.z * distance};
			animated = OscillatingMeshMotionSystemImplementation::BuildCharacterTransform(position, lane.FacingYaw + (std::cos(phase) < 0.0f ? DirectX::XM_PI : 0.0f));
		}
		else
		{
			animated = OscillatingMeshMotionSystemImplementation::BuildCharacterTransform({std::sin(timeSeconds * 0.7f) * 5.25f, 0.0f, 0.0f}, DirectX::XM_PIDIV2);
		}
		const Transform base = WorldTransformConversion::ToPublic(motion.BaseTransform);
		transform = WorldTransformConversion::ToLocal(Transform(base.GetWorldMatrix() * animated));
	}
}
