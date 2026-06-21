#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class RenderBindingSet;
class RenderHardwareInterface;
class Texture;

inline constexpr std::uint32_t MaterialTextureInvalidIndex = UINT32_MAX;

struct MaterialTextureTableBuildResult final
{
	bool Valid = false;
	const char* FailureReason = "not-built";
	std::uint32_t TextureCount = 0u;
};

class MaterialTextureTable final
{
  public:
	MaterialTextureTable() noexcept = default;
	~MaterialTextureTable() noexcept = default;

	MaterialTextureTable(const MaterialTextureTable&) = delete;
	MaterialTextureTable& operator=(const MaterialTextureTable&) = delete;
	MaterialTextureTable(MaterialTextureTable&&) noexcept = default;
	MaterialTextureTable& operator=(MaterialTextureTable&&) noexcept = default;

	void Reset() noexcept;
	std::uint32_t GetOrAddTextureIndex(const Texture* texture);
	MaterialTextureTableBuildResult BuildBindingSet(RenderHardwareInterface& renderHardwareInterface);

	bool IsValid() const noexcept { return m_bindingSet != nullptr && m_textureCount > 0u; }
	const RenderBindingSet* GetBindingSet() const noexcept { return m_bindingSet.get(); }
	std::uint32_t GetTextureCount() const noexcept { return m_textureCount; }

  private:
	std::vector<const Texture*> m_textures;
	std::unique_ptr<RenderBindingSet> m_bindingSet;
	std::uint32_t m_textureCount = 0u;
};
