#pragma once

#include "Scene/SceneObjectSelection.h"
#include "World/WorldChange.h"
#include "World/WorldMaterialVariantView.h"
#include "World/WorldReadView.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct EditorSceneEntry final
{
	std::string Label;
	std::string TypeLabel;
	SceneObjectSelection Selection;
	bool Visible = true;
	SceneLightKind LightKind = SceneLightKind::Unknown;
};

class EditorSceneModel final
{
public:
	std::uint64_t GetModelGeneration() const noexcept { return m_modelGeneration; }
	std::uint64_t GetWorldGeneration() const noexcept { return m_worldGeneration; }
	WorldSequence GetWorldSequence() const noexcept { return m_worldSequence; }
	const std::vector<WorldCameraReadData>& GetCameras() const noexcept { return m_cameras; }
	const std::vector<WorldLightReadData>& GetLights() const noexcept { return m_lights; }
	const std::vector<WorldMeshReadData>& GetMeshes() const noexcept { return m_meshes; }
	const std::optional<SkyEnvironment>& GetSkyEnvironment() const noexcept { return m_sky; }
	const WorldMaterialVariantView& GetMaterialVariants() const noexcept { return m_materialVariants; }
	const std::vector<EditorSceneEntry>& GetCameraEntries() const noexcept { return m_cameraEntries; }
	const std::vector<EditorSceneEntry>& GetLightEntries() const noexcept { return m_lightEntries; }
	const std::vector<EditorSceneEntry>& GetMeshEntries() const noexcept { return m_meshEntries; }
	const EditorSceneEntry& GetSkyEntry() const noexcept { return m_skyEntry; }

	const WorldCameraReadData* FindCamera(EntityId entity) const noexcept;
	const WorldLightReadData* FindLight(EntityId entity) const noexcept;
	const WorldMeshReadData* FindMesh(EntityId entity) const noexcept;
	bool Contains(const SceneObjectSelection& selection) const noexcept;
	const EditorSceneEntry* FindEntry(const SceneObjectSelection& selection) const noexcept;

private:
	friend class EditorSceneModelBuilder;
	void RebuildEntries();
	std::uint64_t m_modelGeneration = 0;
	std::uint64_t m_worldGeneration = 0;
	WorldSequence m_worldSequence = 0;
	std::vector<WorldCameraReadData> m_cameras;
	std::vector<WorldLightReadData> m_lights;
	std::vector<WorldMeshReadData> m_meshes;
	std::optional<SkyEnvironment> m_sky;
	WorldMaterialVariantView m_materialVariants;
	std::vector<EditorSceneEntry> m_cameraEntries;
	std::vector<EditorSceneEntry> m_lightEntries;
	std::vector<EditorSceneEntry> m_meshEntries;
	EditorSceneEntry m_skyEntry;
};
