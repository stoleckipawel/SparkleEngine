#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"

#include <cstddef>
#include <vector>

class SPARKLE_ENGINE_API SceneMaterials final
{
  public:
	SceneMaterials() noexcept = default;
	~SceneMaterials() noexcept = default;

	SceneMaterials(const SceneMaterials&) = delete;
	SceneMaterials& operator=(const SceneMaterials&) = delete;
	SceneMaterials(SceneMaterials&&) = delete;
	SceneMaterials& operator=(SceneMaterials&&) = delete;

	std::size_t GetMaterialCount() const noexcept { return m_materialDescs.size(); }

	const MaterialDesc& GetMaterialDesc(std::size_t index) const noexcept { return m_materialDescs[index]; }
	MaterialDesc& GetMaterialDesc(std::size_t index) noexcept { return m_materialDescs[index]; }

	MaterialHandle AppendMaterials(std::vector<MaterialDesc>&& materialDescs);
	MaterialHandle GetOrCreateDefaultMaterialHandle();
	MaterialSnapshot CaptureSnapshot() const;
	void Reset() noexcept;

  private:
	static MaterialDesc CreateDefaultMaterial();

	std::vector<MaterialDesc> m_materialDescs;
	MaterialHandle m_defaultMaterialHandle = MaterialHandle::Invalid();
};