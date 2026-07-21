#pragma once

#include "Scene/Lighting/SceneLightKind.h"
#include "Util/UiUtil.h"

#include <cstddef>
#include <string>

struct SceneLightDesc;
struct SceneObjectSelection;

namespace SceneObjectPresentation
{
	UiUtil::EditorIcon GetLightIcon(SceneLightKind kind) noexcept;
	const char* GetLightTypeLabel(SceneLightKind kind) noexcept;
	std::string BuildLightLabel(const SceneLightDesc& light, std::size_t lightIndex);
	UiUtil::EditorIcon BuildSelectionIcon(
	    const SceneObjectSelection& selection, SceneLightKind lightKind = SceneLightKind::Unknown) noexcept;
	UiUtil::EditorIcon BuildSelectionIcon(
	    const SceneObjectSelection* selection, SceneLightKind lightKind = SceneLightKind::Unknown) noexcept;
}  // namespace SceneObjectPresentation
