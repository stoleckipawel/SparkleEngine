#pragma once

#include "D3D12/Textures/TextureLoadResult.h"

#include <memory>

class D3D12DescriptorHeapManager;
class D3D12Rhi;
class Texture;

class TextureFactory
{
  public:
	virtual ~TextureFactory() noexcept = default;

	static std::unique_ptr<TextureFactory> Create(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager);

	virtual std::unique_ptr<Texture> CreateTexture(TextureLoadResult loadResult) const = 0;
};