#include "PCH.h"
#include "Scene/Model/EditorSceneModel.h"

#include "Scene/SceneObjectPresentation.h"

#include <algorithm>

class EditorSceneModelOperations final
{
  public:
	template <typename T>
	static const T* FindByEntity(const std::vector<T>& values, EntityId entity) noexcept
	{
		const auto iterator = std::lower_bound(values.begin(), values.end(), entity, [](const T& value, EntityId key) {
			return value.Entity < key;
		});
		return iterator != values.end() && iterator->Entity == entity ? &*iterator : nullptr;
	}

};

const WorldCameraReadData* EditorSceneModel::FindCamera(EntityId entity) const noexcept { return EditorSceneModelOperations::FindByEntity(m_cameras, entity); }
const WorldLightReadData* EditorSceneModel::FindLight(EntityId entity) const noexcept { return EditorSceneModelOperations::FindByEntity(m_lights, entity); }
const WorldMeshReadData* EditorSceneModel::FindMesh(EntityId entity) const noexcept { return EditorSceneModelOperations::FindByEntity(m_meshes, entity); }

bool EditorSceneModel::Contains(const SceneObjectSelection& selection) const noexcept
{
	switch (selection.type)
	{
		case SceneObjectType::Camera: return FindCamera(selection.entity) != nullptr;
		case SceneObjectType::Light: return FindLight(selection.entity) != nullptr;
		case SceneObjectType::Mesh: return FindMesh(selection.entity) != nullptr;
		case SceneObjectType::Sky: return true;
		case SceneObjectType::None: default: return false;
	}
}

const EditorSceneEntry* EditorSceneModel::FindEntry(const SceneObjectSelection& selection) const noexcept
{
	const auto find = [&selection](const std::vector<EditorSceneEntry>& entries) -> const EditorSceneEntry* {
		const auto iterator = std::find_if(entries.begin(), entries.end(), [&selection](const EditorSceneEntry& entry) {
			return entry.Selection == selection;
		});
		return iterator == entries.end() ? nullptr : &*iterator;
	};
	switch (selection.type)
	{
		case SceneObjectType::Camera: return find(m_cameraEntries);
		case SceneObjectType::Light: return find(m_lightEntries);
		case SceneObjectType::Mesh: return find(m_meshEntries);
		case SceneObjectType::Sky: return &m_skyEntry;
		case SceneObjectType::None: default: return nullptr;
	}
}

void EditorSceneModel::RebuildEntries()
{
	m_cameraEntries.clear();
	m_lightEntries.clear();
	m_meshEntries.clear();
	m_cameraEntries.reserve(m_cameras.size());
	m_lightEntries.reserve(m_lights.size());
	m_meshEntries.reserve(m_meshes.size());
	for (std::size_t index = 0; index < m_cameras.size(); ++index)
	{
		const auto& camera = m_cameras[index];
		m_cameraEntries.push_back({camera.Name.empty() ? "Camera " + std::to_string(index + 1) : camera.Name,
		                          "Camera", SceneObjectSelection::Camera(camera.Entity), camera.Visible});
	}
	for (std::size_t index = 0; index < m_lights.size(); ++index)
	{
		const auto& light = m_lights[index];
		m_lightEntries.push_back({SceneObjectPresentation::BuildLightLabel(light.Description, index),
		                         SceneObjectPresentation::GetLightTypeLabel(light.Description.GetKind()),
		                         SceneObjectSelection::Light(light.Entity), light.Description.common.visible,
		                         light.Description.GetKind()});
	}
	for (std::size_t index = 0; index < m_meshes.size(); ++index)
	{
		const auto& mesh = m_meshes[index];
		m_meshEntries.push_back({"Mesh " + std::to_string(index + 1),
		                        mesh.Kind == SceneMeshKind::Skeletal ? "Skeletal Mesh" : "Static Mesh",
		                        SceneObjectSelection::Mesh(mesh.Entity), mesh.Visible});
	}
	m_skyEntry = {m_sky ? "Sky" : "Sky (Engine Default)", "Sky", SceneObjectSelection::Sky(), !m_sky || m_sky->Description.enabled};
}
