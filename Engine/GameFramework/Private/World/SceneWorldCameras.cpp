#include "PCH.h"
#include "World/SceneWorld.h"

#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/SceneWorldTransforms.h"

#include <algorithm>
#include <cmath>

namespace
{
	ECS::Camera ToCameraData(const CameraDesc& desc, float aspectRatio, bool active) noexcept
	{
		return ECS::Camera{
		    .VerticalFieldOfViewDegrees = std::clamp(desc.fovYDegrees, 1.0f, 179.0f),
		    .NearPlane = desc.nearZ,
		    .FarPlane = desc.farZ,
		    .AspectRatio = aspectRatio,
		    .ProjectionKind = desc.projectionKind,
		    .Active = active};
	}

	ECS::CameraMovement ToMovementComponent(const CameraMovementSettings& settings) noexcept
	{
		return ECS::CameraMovement{
		    .MoveSpeed = std::clamp(settings.moveSpeed, settings.minMoveSpeed, settings.maxMoveSpeed),
		    .MinimumMoveSpeed = settings.minMoveSpeed,
		    .MaximumMoveSpeed = settings.maxMoveSpeed,
		    .SpeedStep = settings.speedStep,
		    .SprintMultiplier = settings.sprintMultiplier,
		    .MouseSensitivity = settings.mouseSensitivity,
		    .InvertY = settings.invertY};
	}
}

namespace ECS
{
	EntityId SceneWorld::AddCamera(SceneCameraEntry&& entry, bool active)
	{
		const EntityId entity = m_registry.Create();
		if (!entity.IsValid())
		{
			return entity;
		}

		Transform transform(entry.desc.position, {entry.desc.pitchRadians, entry.desc.yawRadians, 0.0f});
		CameraMovementSettings movement;
		movement.moveSpeed = entry.desc.moveSpeed;
		const LocalTransform local = SceneWorldTransforms::ToLocal(transform);
		const bool added = m_registry.Add(entity, local) &&
		                   m_registry.Add(entity, SceneWorldTransforms::BuildWorld(local)) &&
		                   m_registry.Add(entity, ToCameraData(entry.desc, 16.0f / 9.0f, active)) &&
		                   m_registry.Add(entity, ToMovementComponent(movement)) &&
		                   m_registry.Add(entity, Visibility{}) &&
		                   m_registry.Add(entity, Name{std::move(entry.name)}) &&
		                   m_registry.Add(
		                       entity,
		                       AuthoredIdentity{
		                           .SourceObjectId = ++m_nextCameraIdentity,
		                           .Kind = AuthoredObjectKind::Camera}) &&
		                   m_registry.Add(entity, EditorMetadata{});
		if (!added)
		{
			m_registry.Destroy(entity);
			return EntityId::Invalid();
		}
		if (active || !m_activeCamera.IsValid())
		{
			SetActiveCamera(entity);
		}
		return entity;
	}

	std::size_t SceneWorld::GetCameraCount() const noexcept { return Count<Camera>(); }
	EntityId SceneWorld::GetCameraEntity(std::size_t index) const noexcept { return EntityAt<Camera>(index); }

	std::optional<SceneCameraEntry> SceneWorld::ReadCamera(EntityId entity) const
	{
		const Camera* camera = m_registry.Get<Camera>(entity);
		const LocalTransform* local = m_registry.Get<LocalTransform>(entity);
		if (camera == nullptr || local == nullptr)
		{
			return std::nullopt;
		}
		SceneCameraEntry entry;
		if (const Name* name = m_registry.Get<Name>(entity))
		{
			entry.name = name->Value;
		}
		const Transform transform = SceneWorldTransforms::ToPublic(*local);
		const DirectX::XMFLOAT3 rotation = transform.GetRotationEuler();
		entry.desc.position = transform.GetTranslation();
		entry.desc.pitchRadians = rotation.x;
		entry.desc.yawRadians = rotation.y;
		entry.desc.fovYDegrees = camera->VerticalFieldOfViewDegrees;
		entry.desc.nearZ = camera->NearPlane;
		entry.desc.farZ = camera->FarPlane;
		entry.desc.projectionKind = camera->ProjectionKind;
		entry.desc.moveSpeed = ReadCameraMovement(entity).moveSpeed;
		return entry;
	}

	bool SceneWorld::SetActiveCamera(EntityId entity) noexcept
	{
		const Camera* selected = m_registry.Get<Camera>(entity);
		if (selected == nullptr || selected->ProjectionKind != CameraProjectionKind::Perspective)
		{
			return false;
		}
		if (m_activeCamera.IsValid() && m_activeCamera != entity)
		{
			if (const Camera* previous = m_registry.Get<Camera>(m_activeCamera))
			{
				Camera inactive = *previous;
				inactive.Active = false;
				m_registry.Replace(m_activeCamera, inactive);
			}
		}
		Camera active = *selected;
		active.Active = true;
		if (!m_registry.Replace(entity, active))
		{
			return false;
		}
		m_activeCamera = entity;
		return true;
	}

	bool SceneWorld::WriteCameraDesc(EntityId entity, const CameraDesc& desc) noexcept
	{
		const Camera* existing = m_registry.Get<Camera>(entity);
		if (existing == nullptr)
		{
			return false;
		}
		Transform transform(desc.position, {desc.pitchRadians, desc.yawRadians, 0.0f});
		CameraMovementSettings movement = ReadCameraMovement(entity);
		movement.moveSpeed = desc.moveSpeed;
		return WriteTransform(entity, transform) &&
		       m_registry.Replace(entity, ToCameraData(desc, existing->AspectRatio, existing->Active)) &&
		       WriteCameraMovement(entity, movement);
	}

	bool SceneWorld::WriteCameraMovement(EntityId entity, const CameraMovementSettings& settings) noexcept
	{
		return m_registry.Replace(entity, ToMovementComponent(settings));
	}

	CameraMovementSettings SceneWorld::ReadCameraMovement(EntityId entity) const noexcept
	{
		CameraMovementSettings settings;
		if (const CameraMovement* movement = m_registry.Get<CameraMovement>(entity))
		{
			settings.moveSpeed = movement->MoveSpeed;
			settings.minMoveSpeed = movement->MinimumMoveSpeed;
			settings.maxMoveSpeed = movement->MaximumMoveSpeed;
			settings.speedStep = movement->SpeedStep;
			settings.sprintMultiplier = movement->SprintMultiplier;
			settings.mouseSensitivity = movement->MouseSensitivity;
			settings.invertY = movement->InvertY;
		}
		return settings;
	}

	float SceneWorld::ReadCameraAspectRatio(EntityId entity) const noexcept
	{
		const Camera* camera = m_registry.Get<Camera>(entity);
		return camera != nullptr ? camera->AspectRatio : 1.0f;
	}

	bool SceneWorld::WriteCameraAspectRatio(EntityId entity, float aspectRatio) noexcept
	{
		const Camera* camera = m_registry.Get<Camera>(entity);
		if (camera == nullptr)
		{
			return false;
		}
		Camera updated = *camera;
		updated.AspectRatio = aspectRatio;
		return m_registry.Replace(entity, updated);
	}

	bool SceneWorld::WriteTransform(EntityId entity, const Transform& transform) noexcept
	{
		const LocalTransform local = SceneWorldTransforms::ToLocal(transform);
		return m_registry.Replace(entity, local) && m_registry.Replace(entity, SceneWorldTransforms::BuildWorld(local));
	}

	Transform SceneWorld::ReadTransform(EntityId entity) const noexcept
	{
		const LocalTransform* local = m_registry.Get<LocalTransform>(entity);
		return local == nullptr ? Transform{} : SceneWorldTransforms::ToPublic(*local);
	}

	bool SceneWorld::WriteVisibility(EntityId entity, bool visible) noexcept
	{
		const Visibility* visibility = m_registry.Get<Visibility>(entity);
		if (visibility == nullptr)
		{
			return false;
		}
		Visibility updated = *visibility;
		updated.Visible = visible;
		return m_registry.Replace(entity, updated);
	}

	bool SceneWorld::ReadVisibility(EntityId entity) const noexcept
	{
		const Visibility* visibility = m_registry.Get<Visibility>(entity);
		return visibility != nullptr && visibility->Visible;
	}

	CameraSnapshot SceneWorld::CaptureCamera(EntityId entity) const noexcept
	{
		CameraSnapshot snapshot;
		const Camera* camera = m_registry.Get<Camera>(entity);
		if (camera == nullptr)
		{
			return snapshot;
		}
		const Transform transform = ReadTransform(entity);
		const DirectX::XMFLOAT3 rotation = transform.GetRotationEuler();
		const float cosPitch = std::cos(rotation.x);
		snapshot.position = transform.GetTranslation();
		snapshot.direction = {std::sin(rotation.y) * cosPitch, std::sin(rotation.x), std::cos(rotation.y) * cosPitch};
		snapshot.fovYDegrees = camera->VerticalFieldOfViewDegrees;
		snapshot.aspectRatio = camera->AspectRatio;
		snapshot.nearZ = camera->NearPlane;
		snapshot.farZ = camera->FarPlane;
		return snapshot;
	}
}
