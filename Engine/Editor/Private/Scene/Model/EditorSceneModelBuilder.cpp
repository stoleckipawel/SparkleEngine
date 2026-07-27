#include "PCH.h"
#include "Scene/Model/EditorSceneModelBuilder.h"

#include <algorithm>

class EditorSceneModelPatching final
{
  public:
	template <typename T>
	static void PatchEntity(std::vector<T>& destination, std::span<const T> source, EntityId entity)
	{
		auto destinationIterator = std::lower_bound(destination.begin(), destination.end(), entity,
		    [](const T& value, EntityId key) { return value.Entity < key; });
		const auto sourceIterator = std::lower_bound(source.begin(), source.end(), entity,
		    [](const T& value, EntityId key) { return value.Entity < key; });
		if (sourceIterator != source.end() && sourceIterator->Entity == entity)
		{
			if (destinationIterator != destination.end() && destinationIterator->Entity == entity)
				*destinationIterator = *sourceIterator;
			else
				destination.insert(destinationIterator, *sourceIterator);
		}
		else if (destinationIterator != destination.end() && destinationIterator->Entity == entity)
			destination.erase(destinationIterator);
	}
};

std::shared_ptr<EditorSceneModel> EditorSceneModelBuilder::BuildFull(const WorldReadView& view, std::uint64_t worldGeneration)
{
	auto model = std::make_shared<EditorSceneModel>();
	model->m_modelGeneration = m_nextModelGeneration++;
	model->m_worldGeneration = worldGeneration;
	model->m_worldSequence = view.GetSequence();
	model->m_cameras.assign(view.GetCameras().begin(), view.GetCameras().end());
	model->m_lights.assign(view.GetLights().begin(), view.GetLights().end());
	model->m_meshes.assign(view.GetMeshes().begin(), view.GetMeshes().end());
	model->m_sky = view.GetSkyEnvironment();
	if (m_source.MaterialVariants) model->m_materialVariants = m_source.MaterialVariants();
	model->RebuildEntries();
	return model;
}

std::shared_ptr<EditorSceneModel> EditorSceneModelBuilder::BuildIncremental(
    const WorldReadView& view, const WorldChangeBatch& changes, std::uint64_t worldGeneration)
{
	auto model = std::make_shared<EditorSceneModel>(*m_current);
	model->m_modelGeneration = m_nextModelGeneration++;
	model->m_worldGeneration = worldGeneration;
	model->m_worldSequence = view.GetSequence();
	bool refreshSky = false;
	bool refreshVariants = false;
	for (const WorldChange& change : changes.GetChanges())
	{
		if (change.Kind == WorldChangeKind::WorldReset) return BuildFull(view, worldGeneration);
		if (change.Entity.IsValid())
		{
			EditorSceneModelPatching::PatchEntity(model->m_cameras, view.GetCameras(), change.Entity);
			EditorSceneModelPatching::PatchEntity(model->m_lights, view.GetLights(), change.Entity);
			EditorSceneModelPatching::PatchEntity(model->m_meshes, view.GetMeshes(), change.Entity);
		}
		refreshSky |= change.Data == WorldDataKind::SkyEnvironment;
		refreshVariants |= change.Data == WorldDataKind::Material;
	}
	if (refreshSky) model->m_sky = view.GetSkyEnvironment();
	if (refreshVariants && m_source.MaterialVariants) model->m_materialVariants = m_source.MaterialVariants();
	model->RebuildEntries();
	return model;
}

std::shared_ptr<const EditorSceneModel> EditorSceneModelBuilder::Update()
{
	if (!m_source.AcquireReadView || !m_source.WorldGeneration) return m_current;
	const WorldReadView view = m_source.AcquireReadView();
	if (!view.IsValid()) return m_current;
	const std::uint64_t worldGeneration = m_source.WorldGeneration();
	WorldChangeBatch changes;
	if (m_source.ReadChanges) changes = m_source.ReadChanges(m_cursor);
	const bool full = !m_current || m_current->GetWorldGeneration() != worldGeneration ||
	                  changes.GetStatus() == WorldChangeReadStatus::ResyncRequired;
	if (!full && changes.GetStatus() == WorldChangeReadStatus::UpToDate &&
	    m_current->GetWorldSequence() == view.GetSequence()) return m_current;
	m_current = full ? BuildFull(view, worldGeneration) : BuildIncremental(view, changes, worldGeneration);
	if (m_source.AcknowledgeChanges) (void) m_source.AcknowledgeChanges(m_cursor, view.GetSequence());
	return m_current;
}
