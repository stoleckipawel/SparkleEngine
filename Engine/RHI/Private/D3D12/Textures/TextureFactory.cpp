#include "PCH.h"

#include "D3D12/Textures/TextureFactory.h"

#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Resources/D3D12Texture.h"
#include "Resources/Texture.h"

#include <memory>
#include <utility>

class D3D12TextureFactory final : public TextureFactory
{
  public:
	D3D12TextureFactory(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
	    m_rhi(&rhi), m_descriptorHeapManager(&descriptorHeapManager)
	{
	}

	std::unique_ptr<Texture> CreateTexture(RhiTextureUploadDesc textureUpload) const override
	{
		return std::make_unique<D3D12Texture>(*m_rhi, std::move(textureUpload), *m_descriptorHeapManager);
	}

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};

std::unique_ptr<TextureFactory> TextureFactory::Create(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager)
{
	return std::make_unique<D3D12TextureFactory>(rhi, descriptorHeapManager);
}