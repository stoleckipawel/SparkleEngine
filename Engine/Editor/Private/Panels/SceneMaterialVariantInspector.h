#pragma once

#include <cstdint>

class EditorTransactionManager;
struct WorldMaterialVariantView;

namespace SceneMaterialVariantInspector
{
	void Build(const WorldMaterialVariantView&, EditorTransactionManager&, std::uint64_t) noexcept;
}
