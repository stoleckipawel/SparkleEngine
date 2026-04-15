#pragma once

#include "../RHIAPI.h"
#include "../Formats/CompareOp.h"
#include "../Formats/PixelFormat.h"

#include <d3d12.h>
#include <dxgi1_6.h>

class SPARKLE_RHI_API D3D12TypeConversions final
{
  public:
	static DXGI_FORMAT ToDxgiFormat(PixelFormat format) noexcept;
	static D3D12_COMPARISON_FUNC ToComparisonFunc(CompareOp compareOp) noexcept;

  private:
	D3D12TypeConversions() = delete;
	~D3D12TypeConversions() = delete;
};