#pragma once

#include "RHI/Public/Bindings/RenderBindingSet.h"

#include <memory>

struct PassRuntimeServices;
class Texture;

class EnvironmentMapPassBinding final
{
  public:
	EnvironmentMapPassBinding() noexcept = default;
	~EnvironmentMapPassBinding() noexcept;

	EnvironmentMapPassBinding(const EnvironmentMapPassBinding&) = delete;
	EnvironmentMapPassBinding& operator=(const EnvironmentMapPassBinding&) = delete;
	EnvironmentMapPassBinding(EnvironmentMapPassBinding&&) noexcept = default;
	EnvironmentMapPassBinding& operator=(EnvironmentMapPassBinding&&) noexcept = default;

	RhiDescriptorTableBinding GetTextureBinding(const PassRuntimeServices& passRuntimeServices) const noexcept;

  private:
	mutable std::unique_ptr<RenderBindingSet> m_textureBindingSet;
	mutable const Texture* m_cachedTexture = nullptr;
};
