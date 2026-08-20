#pragma once

#include "Scene/Materials/MaterialData.h"
#include "Scene/Materials/MaterialTextureTable.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class MaterialCache;
class RenderBindingSet;

class RenderMaterialGeneration final
{
public:
	~RenderMaterialGeneration() noexcept;

	RenderMaterialGeneration(const RenderMaterialGeneration&) = delete;
	RenderMaterialGeneration& operator=(const RenderMaterialGeneration&) = delete;
	RenderMaterialGeneration(RenderMaterialGeneration&&) = delete;
	RenderMaterialGeneration& operator=(RenderMaterialGeneration&&) = delete;

	std::span<const MaterialData> GetMaterials() const noexcept { return m_materials; }
	const MaterialTextureTable& GetTextureTable() const noexcept { return m_textureTable; }
	std::uint64_t GetSourceRevision() const noexcept { return m_sourceRevision; }
	std::uint64_t GetTextureRevision() const noexcept { return m_textureRevision; }
	std::uint64_t GetGeneration() const noexcept { return m_generation; }

private:
	friend class MaterialCache;
	RenderMaterialGeneration() noexcept = default;

	std::vector<MaterialData> m_materials;
	std::vector<std::unique_ptr<RenderBindingSet>> m_rasterTextureTables;
	MaterialTextureTable m_textureTable;
	std::uint64_t m_sourceRevision = 0u;
	std::uint64_t m_textureRevision = 0u;
	std::uint64_t m_generation = 0u;
};
