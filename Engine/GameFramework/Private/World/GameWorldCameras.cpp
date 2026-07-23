#include "PCH.h"
#include "World/GameWorldState.h"

#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/WorldTransformConversion.h"

#include <algorithm>

class GameWorldCamerasOperations final
{
  public:
	static ECS::Camera ToCameraData(const CameraDesc& desc, float aspectRatio, bool active) noexcept
	{
		return ECS::Camera{
		    .VerticalFieldOfViewDegrees = std::clamp(desc.fovYDegrees, 1.0f, 179.0f),
		    .NearPlane = desc.nearZ,
		    .FarPlane = desc.farZ,
		    .AspectRatio = aspectRatio,
		    .ProjectionKind = desc.projectionKind,
		    .Active = active};
	}

	static ECS::CameraMovement ToMovementComponent(const CameraMovementSettings& settings) noexcept
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
};

namespace ECS
{
	EntityId GameWorldState::AddCamera(SceneCameraEntry&& entry, bool active)
	{
		const EntityId entity = m_registry.Create();
		if (!entity.IsValid())
		{
			return entity;
		}

		Transform transform(entry.desc.position, {entry.desc.pitchRadians, entry.desc.yawRadians, 0.0f});
		CameraMovementSettings movement;
		movement.moveSpeed = entry.desc.moveSpeed;
		const LocalTransform local = WorldTransformConversion::ToLocal(transform);
		const bool added = m_registry.Add(entity, local) &&
		                   m_registry.Add(entity, WorldTransform{}) &&
		                   m_registry.Add(entity, GameWorldCamerasOperations::ToCameraData(entry.desc, 16.0f / 9.0f, active)) &&
		                   m_registry.Add(entity, CameraDerivedState{}) &&
		                   m_registry.Add(entity, GameWorldCamerasOperations::ToMovementComponent(movement)) &&
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
		MarkTransformDirty(entity);
		RecordChange(entity, WorldChangeKind::EntityCreated, WorldDataKind::World);
		RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::Camera);
		RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::LocalTransform);
		if (active || !m_activeCamera.IsValid())
		{
			SetActiveCamera(entity);
		}
		return entity;
	}

	std::size_t GameWorldState::GetCameraCount() const noexcept { return Count<Camera>(); }
	EntityId GameWorldState::GetCameraEntity(std::size_t index) const noexcept { return EntityAt<Camera>(index); }
	bool GameWorldState::IsCamera(EntityId entity) const noexcept { return m_registry.Get<Camera>(entity) != nullptr; }

	std::optional<SceneCameraEntry> GameWorldState::ReadCamera(EntityId entity) const
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
		const Transform transform = WorldTransformConversion::ToPublic(*local);
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

	bool GameWorldState::SetActiveCamera(EntityId entity) noexcept
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
				RecordChange(m_activeCamera, WorldChangeKind::ValueChanged, WorldDataKind::Camera);
			}
		}
		Camera active = *selected;
		active.Active = true;
		if (!m_registry.Replace(entity, active))
		{
			return false;
		}
		m_activeCamera = entity;
		RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::Camera);
		return true;
	}

	bool GameWorldState::WriteCameraDesc(EntityId entity, const CameraDesc& desc) noexcept
	{
		const Camera* existing = m_registry.Get<Camera>(entity);
		if (existing == nullptr)
		{
			return false;
		}
		Transform transform(desc.position, {desc.pitchRadians, desc.yawRadians, 0.0f});
		CameraMovementSettings movement = ReadCameraMovement(entity);
		movement.moveSpeed = desc.moveSpeed;
		const bool written = WriteTransform(entity, transform) &&
		                     m_registry.Replace(entity, GameWorldCamerasOperations::ToCameraData(desc, existing->AspectRatio, existing->Active)) &&
		                     WriteCameraMovement(entity, movement);
		if (written)
		{
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::Camera);
		}
		return written;
	}

	bool GameWorldState::WriteCameraMovement(EntityId entity, const CameraMovementSettings& settings) noexcept
	{
		const bool written = m_registry.Replace(entity, GameWorldCamerasOperations::ToMovementComponent(settings));
		if (written)
		{
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::CameraMovement);
		}
		return written;
	}

	CameraMovementSettings GameWorldState::ReadCameraMovement(EntityId entity) const noexcept
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

	float GameWorldState::ReadCameraAspectRatio(EntityId entity) const noexcept
	{
		const Camera* camera = m_registry.Get<Camera>(entity);
		return camera != nullptr ? camera->AspectRatio : 1.0f;
	}

	bool GameWorldState::WriteCameraAspectRatio(EntityId entity, float aspectRatio) noexcept
	{
		const Camera* camera = m_registry.Get<Camera>(entity);
		if (camera == nullptr)
		{
			return false;
		}
		Camera updated = *camera;
		updated.AspectRatio = aspectRatio;
		const bool written = m_registry.Replace(entity, updated);
		if (written)
		{
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::Camera);
		}
		return written;
	}

	bool GameWorldState::WriteTransform(EntityId entity, const Transform& transform) noexcept
	{
		const bool written = m_registry.Replace(entity, WorldTransformConversion::ToLocal(transform));
		if (written)
		{
			MarkTransformDirty(entity);
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::LocalTransform);
		}
		return written;
	}

	Transform GameWorldState::ReadTransform(EntityId entity) const noexcept
	{
		const LocalTransform* local = m_registry.Get<LocalTransform>(entity);
		return local == nullptr ? Transform{} : WorldTransformConversion::ToPublic(*local);
	}

	bool GameWorldState::WriteVisibility(EntityId entity, bool visible) noexcept
	{
		const Visibility* visibility = m_registry.Get<Visibility>(entity);
		if (visibility == nullptr)
		{
			return false;
		}
		Visibility updated = *visibility;
		updated.Visible = visible;
		const bool written = m_registry.Replace(entity, updated);
		if (written)
		{
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::Visibility);
		}
		return written;
	}

	bool GameWorldState::ReadVisibility(EntityId entity) const noexcept
	{
		const Visibility* visibility = m_registry.Get<Visibility>(entity);
		return visibility != nullptr && visibility->Visible;
	}

}
