#pragma once

#include "Scene/Lighting/DirectionalLight.h"
#include "Scene/Lighting/PointLight.h"
#include "Scene/Lighting/RectLight.h"
#include "Scene/Lighting/SpotLight.h"
#include "Rendering/RenderSceneDynamicData.h"

#include <variant>
#include <span>

struct PreparedRenderScene;

struct PreparedRenderLight final
{
	RenderObjectId Object;
	std::variant<std::monostate, DirectionalLight, PointLight, SpotLight, RectLight> Payload;
};

void PrepareRenderLights(std::span<const RenderLightData> inputs, std::span<PreparedRenderLight> outputs) noexcept;
void CommitPreparedRenderLights(std::span<const PreparedRenderLight> lights, PreparedRenderScene& preparedScene);
