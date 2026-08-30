#pragma once

#include "World/WorldEditCommand.h"

#include <optional>
#include <string>

class EditorSceneModel;
struct SceneObjectSelection;

struct EditorCommandPair final
{
	WorldEditCommand Forward;
	WorldEditCommand Inverse;
	std::string CoalescingKey;
};

// Pure translation from an immutable editor model and user intent to semantic world commands.
// Panels own widget behavior only and never reconstruct domain mutation rules.
class SceneObjectCommandFactory final
{
public:
	static std::optional<EditorCommandPair> SetVisibility(
	    const EditorSceneModel& model,
	    const SceneObjectSelection& selection,
	    bool visible);
	static std::optional<EditorCommandPair> SetActiveCamera(const EditorSceneModel& model, EntityId camera);
};
