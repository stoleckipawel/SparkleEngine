#pragma once

#include "Scene/Model/EditorSceneModel.h"

#include <functional>
#include <memory>

struct EditorSceneSource final
{
	std::function<WorldReadView()> AcquireReadView;
	std::function<WorldChangeBatch(const WorldChangeCursor&)> ReadChanges;
	std::function<bool(WorldChangeCursor&, WorldSequence)> AcknowledgeChanges;
	std::function<std::uint64_t()> WorldGeneration;
	std::function<WorldMaterialVariantView()> MaterialVariants;
};

class EditorSceneModelBuilder final
{
public:
	explicit EditorSceneModelBuilder(EditorSceneSource source) :
	    m_source(std::move(source))
	{
	}
	std::shared_ptr<const EditorSceneModel> Update();
	std::shared_ptr<const EditorSceneModel> GetCurrent() const noexcept { return m_current; }

private:
	std::shared_ptr<EditorSceneModel> BuildFull(const WorldReadView& view, std::uint64_t worldGeneration);
	std::shared_ptr<EditorSceneModel> BuildIncremental(
	    const WorldReadView& view,
	    const WorldChangeBatch& changes,
	    std::uint64_t worldGeneration);
	EditorSceneSource m_source;
	WorldChangeCursor m_cursor;
	std::shared_ptr<const EditorSceneModel> m_current;
	std::uint64_t m_nextModelGeneration = 1;
};
