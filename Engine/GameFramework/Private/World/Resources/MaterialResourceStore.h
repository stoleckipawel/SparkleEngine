#pragma once

#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Rendering/RenderInputFrame.h"

#include <cstdint>
#include <vector>

class MaterialResourceStore final
{
  public:
	explicit MaterialResourceStore(std::uint32_t generation) noexcept : m_generation(generation) {}
	MaterialHandle Append(std::vector<MaterialDesc>&& descriptions);
	MaterialHandle GetOrCreateDefault();
	bool Contains(MaterialHandle handle) const noexcept;
	std::uint64_t GetContentRevision() const noexcept { return m_contentRevision; }
	RenderMaterialTable CaptureRenderTable() const;

  private:
	static MaterialDesc CreateDefault();
	std::vector<MaterialDesc> m_descriptions;
	MaterialHandle m_default = MaterialHandle::Invalid();
	std::uint32_t m_generation = 0;
	std::uint64_t m_contentRevision = 0;
};
