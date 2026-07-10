#include "../../PCH.h"
#include "Passes/Bindings/EnvironmentMapPassBinding.h"

#include "FrameGraph/PassRuntimeServices.h"
#include "RHI/Public/Resources/Texture.h"
#include "Textures/TextureManager.h"

EnvironmentMapPassBinding::~EnvironmentMapPassBinding() noexcept = default;

RhiDescriptorTableBinding EnvironmentMapPassBinding::GetTextureBinding(const PassRuntimeServices& passRuntimeServices) const noexcept
{
	const Texture* environmentTexture =
	    passRuntimeServices.Textures != nullptr ? passRuntimeServices.Textures->ResolveEnvironmentMapTexture() : nullptr;
	if (environmentTexture == nullptr)
	{
		return {};
	}

	RenderHardwareInterface& renderHardwareInterface = passRuntimeServices.HardwareInterface;
	if (!m_textureBindingSet)
	{
		m_textureBindingSet = renderHardwareInterface.GetDescriptorService().CreateBindingSet(
		    RenderBindingSetDesc{.DescriptorType = ERhiDescriptorAllocatorType::ShaderResource, .DescriptorCount = 1u});
	}

	if (!m_textureBindingSet || !*m_textureBindingSet)
	{
		return {};
	}

	if (m_cachedTexture != environmentTexture)
	{
		environmentTexture->WriteShaderResourceView(m_textureBindingSet->GetCpuDescriptorHandle());
		m_cachedTexture = environmentTexture;
	}

	return m_textureBindingSet->GetTableBinding();
}
