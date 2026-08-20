#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

class RenderBindingSet;
class RenderHardwareInterface;

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
	std::uint32_t GetOrAddTextureIndex(RhiResourceViewHandle textureView);
	void BuildBindingSet(RenderHardwareInterface& renderHardwareInterface);

	bool IsValid() const noexcept { return m_bindingSet != nullptr && !m_textureViews.empty(); }
	RhiDescriptorTableBinding GetTableBinding() const noexcept;
	std::uint32_t GetTextureCount() const noexcept { return static_cast<std::uint32_t>(m_textureViews.size()); }

private:
	std::vector<RhiResourceViewHandle> m_textureViews;
	std::unique_ptr<RenderBindingSet> m_bindingSet;
};
