#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

class SceneMeshes;

using SceneMaterialVariantIndex = std::uint32_t;

inline constexpr SceneMaterialVariantIndex kInvalidSceneMaterialVariantIndex =
    (std::numeric_limits<SceneMaterialVariantIndex>::max)();

struct SPARKLE_ENGINE_API SceneMaterialVariantDesc
{
	std::string name;
	std::uint32_t sourceVariantIndex = 0;
};

struct SPARKLE_ENGINE_API SceneMaterialVariantBinding
{
	SceneMaterialVariantIndex variantIndex = kInvalidSceneMaterialVariantIndex;
	SceneMeshInstanceIndex meshInstanceIndex = kInvalidSceneMeshInstanceIndex;
	MaterialHandle material;
};

class SPARKLE_ENGINE_API SceneMaterialVariants final
{
  public:
	std::size_t GetVariantCount() const noexcept { return m_variants.size(); }
	const SceneMaterialVariantDesc& GetVariant(std::size_t variantIndex) const noexcept { return m_variants[variantIndex]; }
	SceneMaterialVariantIndex GetActiveVariantIndex() const noexcept { return m_activeVariantIndex; }

	void AppendVariants(std::vector<SceneMaterialVariantDesc>&& variants, std::vector<SceneMaterialVariantBinding>&& bindings);
	bool ApplyVariant(SceneMaterialVariantIndex variantIndex, SceneMeshes& meshes);
	void Reset() noexcept;

  private:
	std::vector<SceneMaterialVariantDesc> m_variants;
	std::vector<SceneMaterialVariantBinding> m_bindings;
	SceneMaterialVariantIndex m_activeVariantIndex = kInvalidSceneMaterialVariantIndex;
};
