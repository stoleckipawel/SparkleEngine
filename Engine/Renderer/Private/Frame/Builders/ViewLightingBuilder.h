#pragma once

struct PerViewLightingConstantBufferData;
struct RenderSceneData;

class ViewLightingBuilder final
{
  public:
	ViewLightingBuilder() noexcept = default;
	~ViewLightingBuilder() noexcept = default;

	ViewLightingBuilder(const ViewLightingBuilder&) = delete;
	ViewLightingBuilder& operator=(const ViewLightingBuilder&) = delete;
	ViewLightingBuilder(ViewLightingBuilder&&) = delete;
	ViewLightingBuilder& operator=(ViewLightingBuilder&&) = delete;

	PerViewLightingConstantBufferData Build(const RenderSceneData& sceneData) const noexcept;
};
