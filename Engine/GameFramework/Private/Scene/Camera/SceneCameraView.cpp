#include "PCH.h"
#include "Scene/Camera/SceneCameraView.h"

#include "Scene/GameScene.h"
#include "World/SceneWorld.h"

bool SceneCameraView::IsValid() const noexcept
{
	return m_scene != nullptr && m_scene->m_world->ReadCamera(m_entity).has_value();
}

CameraDesc SceneCameraView::GetDesc() const noexcept
{
	if (m_scene == nullptr)
	{
		return {};
	}
	const std::optional<SceneCameraEntry> entry = m_scene->m_world->ReadCamera(m_entity);
	return entry ? entry->desc : CameraDesc{};
}

void SceneCameraView::SetDesc(const CameraDesc& desc) noexcept
{
	if (m_scene != nullptr)
	{
		m_scene->m_world->WriteCameraDesc(m_entity, desc);
	}
}

CameraMovementSettings SceneCameraView::GetMovementSettings() const noexcept
{
	return m_scene != nullptr ? m_scene->m_world->ReadCameraMovement(m_entity) : CameraMovementSettings{};
}

void SceneCameraView::SetMovementSettings(const CameraMovementSettings& settings) noexcept
{
	if (m_scene != nullptr)
	{
		m_scene->m_world->WriteCameraMovement(m_entity, settings);
	}
}

Transform SceneCameraView::GetTransform() const noexcept
{
	return m_scene != nullptr ? m_scene->m_world->ReadTransform(m_entity) : Transform{};
}

void SceneCameraView::SetTransform(const Transform& transform) noexcept
{
	if (m_scene != nullptr)
	{
		m_scene->m_world->WriteTransform(m_entity, transform);
	}
}

bool SceneCameraView::IsVisible() const noexcept
{
	return m_scene != nullptr && m_scene->m_world->ReadVisibility(m_entity);
}

void SceneCameraView::SetVisible(bool visible) noexcept
{
	if (m_scene != nullptr)
	{
		m_scene->m_world->WriteVisibility(m_entity, visible);
	}
}

float SceneCameraView::GetAspectRatio() const noexcept
{
	return m_scene != nullptr ? m_scene->m_world->ReadCameraAspectRatio(m_entity) : 1.0f;
}

void SceneCameraView::SetAspectRatio(float aspectRatio) noexcept
{
	if (m_scene != nullptr)
	{
		m_scene->m_world->WriteCameraAspectRatio(m_entity, aspectRatio);
	}
}
