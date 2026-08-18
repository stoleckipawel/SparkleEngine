#include "PCH.h"

#include "World/Systems/CameraMovementSystem.h"

#include "GameFramework/Public/Scene/Camera/CameraNavigation.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"

#include <algorithm>
#include <cmath>

namespace ECS
{
	CameraMovementSystem::CameraMovementSystem(EntityId activeCamera, const CameraSimulationInput& input) noexcept :
	    m_activeCamera(activeCamera),
	    m_intent(input.Intent),
	    m_navigationSettings(input.NavigationSettings),
	    m_deltaSeconds((std::max) (0.0f, input.DeltaSeconds))
	{
	}

	bool CameraMovementSystem::Apply(EntityId entity, Camera& camera, LocalTransform& transform) const noexcept
	{
		if (entity != m_activeCamera || !camera.Active)
			return false;
		bool changed = false;
		if (m_intent.HasAspectRatio && m_intent.AspectRatio > 0.0f && camera.AspectRatio != m_intent.AspectRatio)
		{
			camera.AspectRatio = m_intent.AspectRatio;
			changed = true;
		}

		CameraNavigationState navigationState;
		navigationState.Position = transform.Translation;
		const float sinPitch = 2.0f * (transform.Rotation.w * transform.Rotation.x - transform.Rotation.z * transform.Rotation.y);
		navigationState.PitchRadians = std::asin(std::clamp(sinPitch, -1.0f, 1.0f));
		navigationState.YawRadians = std::atan2(
		    2.0f * (transform.Rotation.w * transform.Rotation.y + transform.Rotation.x * transform.Rotation.z),
		    1.0f - 2.0f * (transform.Rotation.x * transform.Rotation.x + transform.Rotation.y * transform.Rotation.y));
		if (CameraNavigation::Apply(m_intent, m_navigationSettings, m_deltaSeconds, navigationState))
		{
			transform.Translation = navigationState.Position;
			DirectX::XMStoreFloat4(
			    &transform.Rotation,
			    DirectX::XMQuaternionRotationRollPitchYaw(navigationState.PitchRadians, navigationState.YawRadians, 0.0f));
			changed = true;
		}
		return changed;
	}
}
