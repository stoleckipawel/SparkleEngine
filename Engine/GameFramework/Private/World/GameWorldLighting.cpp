#include "PCH.h"
#include "World/GameWorldState.h"

#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/WorldTransformConversion.h"

class GameWorldLightingOperations final
{
  public:
	static ECS::Light ToLightComponent(const SceneLightDesc& desc) noexcept
	{
		ECS::Light light;
		light.Kind = desc.GetKind();
		light.Color = desc.common.color;
		light.Intensity = desc.common.intensity;
		if (const SceneDirectionalLightDesc* directional = desc.GetDirectional())
		{
			light.Direction = directional->direction;
			light.AngularDiameterRadians = directional->angularDiameterRadians;
			light.CastShadow = directional->castShadow;
		}
		else if (const PointLightDesc* point = desc.GetPoint())
		{
			light.Range = point->range;
			light.SourceRadius = point->sourceRadius;
			light.CastShadow = point->castShadow;
		}
		else if (const SpotLightDesc* spot = desc.GetSpot())
		{
			light.Direction = spot->direction;
			light.Range = spot->range;
			light.SourceRadius = spot->sourceRadius;
			light.InnerConeRadians = spot->innerConeAngleRadians;
			light.OuterConeRadians = spot->outerConeAngleRadians;
			light.CastShadow = spot->castShadow;
		}
		else if (const RectLightDesc* rect = desc.GetRect())
		{
			light.Direction = rect->direction;
			light.Tangent = rect->tangent;
			light.AreaSize = {rect->width, rect->height};
			light.CastShadow = rect->castShadow;
		}
		return light;
	}

	static SceneLightPayload ToLightPayload(const ECS::Light& light)
	{
		switch (light.Kind)
		{
			case SceneLightKind::Directional:
				return SceneDirectionalLightDesc{
				    .direction = light.Direction,
				    .angularDiameterRadians = light.AngularDiameterRadians,
				    .castShadow = light.CastShadow};
			case SceneLightKind::Point:
				return PointLightDesc{.range = light.Range, .sourceRadius = light.SourceRadius, .castShadow = light.CastShadow};
			case SceneLightKind::Spot:
				return SpotLightDesc{
				    .direction = light.Direction,
				    .range = light.Range,
				    .sourceRadius = light.SourceRadius,
				    .innerConeAngleRadians = light.InnerConeRadians,
				    .outerConeAngleRadians = light.OuterConeRadians,
				    .castShadow = light.CastShadow};
			case SceneLightKind::Rect:
				return RectLightDesc{
				    .direction = light.Direction,
				    .width = light.AreaSize.x,
				    .tangent = light.Tangent,
				    .height = light.AreaSize.y,
				    .castShadow = light.CastShadow};
			case SceneLightKind::Unknown:
			default:
				return std::monostate{};
		}
	}
};

namespace ECS
{
	EntityId GameWorldState::AddLight(SceneLightDesc&& desc)
	{
		const EntityId entity = m_registry.Create();
		if (!entity.IsValid())
		{
			return entity;
		}
		const Transform transform(DirectX::XMLoadFloat4x4(&desc.common.worldTransform));
		const LocalTransform local = WorldTransformConversion::ToLocal(transform);
		const bool added = m_registry.Add(entity, local) &&
		                   m_registry.Add(entity, WorldTransform{}) &&
		                   m_registry.Add(entity, GameWorldLightingOperations::ToLightComponent(desc)) &&
		                   m_registry.Add(entity, Visibility{.Visible = desc.common.visible}) &&
		                   m_registry.Add(entity, Name{std::move(desc.common.name)}) &&
		                   m_registry.Add(
		                       entity,
		                       AuthoredIdentity{
		                           .SourceObjectId = ++m_nextLightIdentity,
		                           .Kind = AuthoredObjectKind::Light}) &&
		                   m_registry.Add(entity, EditorMetadata{});
		if (!added)
		{
			m_registry.Destroy(entity);
			return EntityId::Invalid();
		}
		MarkTransformDirty(entity);
		RecordChange(entity, WorldChangeKind::EntityCreated, WorldDataKind::World);
		RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::Light);
		RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::LocalTransform);
		return entity;
	}

	std::size_t GameWorldState::GetLightCount() const noexcept { return Count<Light>(); }
	EntityId GameWorldState::GetLightEntity(std::size_t index) const noexcept { return EntityAt<Light>(index); }

	std::optional<SceneLightDesc> GameWorldState::ReadLight(EntityId entity) const
	{
		const Light* light = m_registry.Get<Light>(entity);
		if (light == nullptr)
		{
			return std::nullopt;
		}
		SceneLightDesc desc;
		if (const Name* name = m_registry.Get<Name>(entity))
		{
			desc.common.name = name->Value;
		}
		if (const WorldTransform* transform = m_registry.Get<WorldTransform>(entity))
		{
			desc.common.worldTransform = transform->Matrix;
		}
		desc.common.color = light->Color;
		desc.common.intensity = light->Intensity;
		desc.common.visible = ReadVisibility(entity);
		desc.payload = GameWorldLightingOperations::ToLightPayload(*light);
		return desc;
	}

	bool GameWorldState::WriteLight(EntityId entity, SceneLightDesc&& desc)
	{
		if (m_registry.Get<Light>(entity) == nullptr)
		{
			return false;
		}
		const Transform transform(DirectX::XMLoadFloat4x4(&desc.common.worldTransform));
		const bool written = WriteTransform(entity, transform) &&
		                     m_registry.Replace(entity, GameWorldLightingOperations::ToLightComponent(desc)) &&
		                     m_registry.Replace(entity, Name{std::move(desc.common.name)}) &&
		                     WriteVisibility(entity, desc.common.visible);
		if (written)
		{
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::Light);
		}
		return written;
	}

	std::vector<SceneLightDesc> GameWorldState::CaptureLightsToDesc() const
	{
		std::vector<SceneLightDesc> lights;
		lights.reserve(GetLightCount());
		for (std::size_t index = 0; index < GetLightCount(); ++index)
		{
			if (std::optional<SceneLightDesc> light = ReadLight(GetLightEntity(index)))
			{
				lights.push_back(std::move(*light));
			}
		}
		return lights;
	}

}
