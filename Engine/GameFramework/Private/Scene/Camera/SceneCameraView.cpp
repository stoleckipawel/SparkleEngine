#include "PCH.h"
#include "Scene/Camera/SceneCameraView.h"

#include "World/GameWorld.h"
#include "World/GameWorldState.h"

bool SceneCameraView::IsValid() const noexcept
{
	return m_world != nullptr && m_world->m_state->ReadCamera(m_entity).has_value();
}

CameraDesc SceneCameraView::GetDesc() const noexcept
{
	if (m_world == nullptr)
	{
		return {};
	}
	const std::optional<SceneCameraEntry> entry = m_world->m_state->ReadCamera(m_entity);
	return entry ? entry->desc : CameraDesc{};
}

void SceneCameraView::SetDesc(const CameraDesc& desc) noexcept
{
	if (m_world != nullptr)
	{
		m_world->m_state->WriteCameraDesc(m_entity, desc);
	}
}

CameraMovementSettings SceneCameraView::GetMovementSettings() const noexcept
{
	return m_world != nullptr ? m_world->m_state->ReadCameraMovement(m_entity) : CameraMovementSettings{};
}

void SceneCameraView::SetMovementSettings(const CameraMovementSettings& settings) noexcept
{
	if (m_world != nullptr)
	{
		m_world->m_state->WriteCameraMovement(m_entity, settings);
	}
}

Transform SceneCameraView::GetTransform() const noexcept
{
	return m_world != nullptr ? m_world->m_state->ReadTransform(m_entity) : Transform{};
}

void SceneCameraView::SetTransform(const Transform& transform) noexcept
{
	if (m_world != nullptr)
	{
		m_world->m_state->WriteTransform(m_entity, transform);
	}
}

bool SceneCameraView::IsVisible() const noexcept
{
	return m_world != nullptr && m_world->m_state->ReadVisibility(m_entity);
}

void SceneCameraView::SetVisible(bool visible) noexcept
{
	if (m_world != nullptr)
	{
		m_world->m_state->WriteVisibility(m_entity, visible);
	}
}

float SceneCameraView::GetAspectRatio() const noexcept
{
	return m_world != nullptr ? m_world->m_state->ReadCameraAspectRatio(m_entity) : 1.0f;
}

void SceneCameraView::SetAspectRatio(float aspectRatio) noexcept
{
	if (m_world != nullptr)
	{
		m_world->m_state->WriteCameraAspectRatio(m_entity, aspectRatio);
	}
}
