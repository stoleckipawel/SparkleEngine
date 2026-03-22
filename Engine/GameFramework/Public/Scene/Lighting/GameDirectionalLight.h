#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"

#include <DirectXMath.h>

class SPARKLE_ENGINE_API GameDirectionalLight final
{
  public:
	GameDirectionalLight() noexcept = default;

	static DirectionalLightDesc SanitizeDesc(const DirectionalLightDesc& desc) noexcept;

	explicit GameDirectionalLight(const DirectionalLightDesc& desc) noexcept : m_desc(SanitizeDesc(desc)) {}

	const DirectionalLightDesc& GetDesc() const noexcept { return m_desc; }

	void ApplyDesc(const DirectionalLightDesc& desc) noexcept { m_desc = SanitizeDesc(desc); }

  private:
	DirectionalLightDesc m_desc;
};