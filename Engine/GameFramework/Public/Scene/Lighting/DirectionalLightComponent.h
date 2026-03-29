#pragma once

#include "GameFramework/Public/Scene/Component.h"
#include "GameFramework/Public/Scene/Lighting/DirectionalLightDesc.h"

class SPARKLE_ENGINE_API DirectionalLightComponent final : public Component
{
  public:
	DirectionalLightComponent() noexcept = default;
	~DirectionalLightComponent() override = default;

	static DirectionalLightDesc SanitizeDesc(const DirectionalLightDesc& desc) noexcept;

	explicit DirectionalLightComponent(const DirectionalLightDesc& desc) noexcept : m_desc(SanitizeDesc(desc)) {}

	const DirectionalLightDesc& GetDesc() const noexcept { return m_desc; }

	void ApplyDesc(const DirectionalLightDesc& desc) noexcept { m_desc = SanitizeDesc(desc); }

  private:
	DirectionalLightDesc m_desc;
};