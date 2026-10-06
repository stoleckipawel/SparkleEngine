#pragma once

#include "Rendering/RenderResourceTables.h"

#include <cstdint>
#include <memory>

struct PreparedRenderScene;
class RenderBindingSet;
class RenderHardwareInterface;
class RenderMaterialGeneration;
class TextureCache;
struct MaterialDesc;

class MaterialCache final
{
public:
	MaterialCache(TextureCache& textureCache, RenderHardwareInterface& renderHardwareInterface) noexcept;
	~MaterialCache() noexcept;

	MaterialCache(const MaterialCache&) = delete;
	MaterialCache& operator=(const MaterialCache&) = delete;
	MaterialCache(MaterialCache&&) = delete;
	MaterialCache& operator=(MaterialCache&&) = delete;

	void BuildMaterials(const RenderMaterialTable& materials, std::uint64_t sourceRevision, PreparedRenderScene& preparedScene);
	void Reset() noexcept;

private:
	void Rebuild(const RenderMaterialTable& materials, std::uint64_t sourceRevision, std::uint64_t textureRevision);
	void BuildMaterial(const MaterialDesc& desc, std::uint32_t materialIndex, std::uint64_t generation, RenderMaterialGeneration& output);
	void PublishMaterialTextureTable(PreparedRenderScene& preparedScene) const noexcept;
	std::uint64_t GetNextGeneration() const noexcept;

	TextureCache& m_textureCache;
	RenderHardwareInterface& m_renderHardwareInterface;
	std::shared_ptr<const RenderMaterialGeneration> m_currentGeneration;
};
