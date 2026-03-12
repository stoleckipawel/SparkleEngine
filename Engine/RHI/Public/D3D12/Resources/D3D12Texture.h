// ============================================================================
// D3D12Texture.h
// ----------------------------------------------------------------------------
// Manages upload and GPU resource creation for 2D textures.
//
#pragma once

#include "TexturePayload.h"
#include "D3D12DescriptorHandle.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12DescriptorHeapManager;
class D3D12Rhi;

class D3D12Texture
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	/// Constructs a texture from a runtime payload with explicit mip data.
	/// @param rhi Reference to the D3D12 RHI for device access.
	/// @param texturePayload CPU-side texture payload containing all mip levels.
	/// @param descriptorHeapManager Reference to the descriptor heap manager for SRV allocation.
	D3D12Texture(
	    D3D12Rhi& rhi,
	    TexturePayload texturePayload,
	    D3D12DescriptorHeapManager& descriptorHeapManager);

	/// Releases the SRV descriptor slot.
	~D3D12Texture() noexcept;

	// Non-copyable: descriptor ownership cannot be shared
	D3D12Texture(const D3D12Texture&) = delete;
	D3D12Texture& operator=(const D3D12Texture&) = delete;

	// Non-movable: prevents accidental transfer leading to double-free
	D3D12Texture(D3D12Texture&&) = delete;
	D3D12Texture& operator=(D3D12Texture&&) = delete;

	// ========================================================================
	// Accessors
	// ========================================================================

	/// Returns the GPU descriptor handle for shader binding.
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_srvHandle.GetGPU(); }

	/// Returns the CPU descriptor handle (for copy operations).
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const noexcept { return m_srvHandle.GetCPU(); }

	/// Writes an SRV for this texture into the provided CPU descriptor slot.
	void WriteShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE destination) const;

  private:
	// ------------------------------------------------------------------------
	// Initialization Helpers
	// ------------------------------------------------------------------------

	/// Creates the committed GPU texture resource.
	void CreateResource();

	/// Copies texture data from CPU to GPU via upload buffer.
	void UploadToGPU();

	/// Allocates SRV descriptor and creates the view.
	void CreateShaderResourceView();
	D3D12_SHADER_RESOURCE_VIEW_DESC BuildShaderResourceViewDesc() const noexcept;

	// ------------------------------------------------------------------------
	// Resources
	// ------------------------------------------------------------------------

	D3D12Rhi& m_rhi;                                                ///< RHI reference
	ComPtr<ID3D12Resource2> m_textureResource;                      ///< GPU texture resource (default heap)
	ComPtr<ID3D12Resource2> m_uploadResource;                       ///< Upload buffer (upload heap)
	TexturePayload m_texturePayload;                                ///< CPU-side payload for upload/build metadata
	D3D12DescriptorHandle m_srvHandle;                              ///< SRV descriptor handle
	D3D12_RESOURCE_DESC m_texResourceDesc = {};                     ///< Texture resource description
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;  ///< Descriptor heap manager reference
};
