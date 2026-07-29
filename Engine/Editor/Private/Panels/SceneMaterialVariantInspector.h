#pragma once

#include <cstdint>

class EditorTransactionHistory;
struct WorldMaterialVariantView;

namespace SceneMaterialVariantInspector
{
	void Build(const WorldMaterialVariantView&, EditorTransactionHistory&, std::uint64_t) noexcept;
}
