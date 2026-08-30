#pragma once

#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Materials/MaterialVariant.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/World/EntityId.h"

#include <string_view>
#include <vector>

namespace ECS
{
	class GameWorldState;
}

struct MaterialVariantBinding final
{
	MaterialVariantIndex Variant = kInvalidMaterialVariantIndex;
	EntityId Entity;
	MaterialHandle Material;
};

class MaterialVariantResourceStore final
{
public:
	void Append(std::vector<MaterialVariantDesc>&& variants, std::vector<MaterialVariantBinding>&& bindings);
	bool Apply(MaterialVariantIndex index, ECS::GameWorldState& world);
	std::size_t GetCount() const noexcept { return m_variants.size(); }
	std::string_view GetName(std::size_t index) const noexcept;
	MaterialVariantIndex GetActive() const noexcept { return m_active; }

private:
	std::vector<MaterialVariantDesc> m_variants;
	std::vector<MaterialVariantBinding> m_bindings;
	MaterialVariantIndex m_active = kInvalidMaterialVariantIndex;
};
