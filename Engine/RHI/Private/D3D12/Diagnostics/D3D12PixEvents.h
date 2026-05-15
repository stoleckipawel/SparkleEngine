#pragma once

#include "Device/RenderHardwareInterface.h"

#include <d3d12.h>

namespace D3D12PixEvents
{
	bool IsAvailable() noexcept;
	UINT64 ToColor(RhiDiagnosticLabelColor color) noexcept;
	void BeginEvent(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) noexcept;
	void EndEvent(ID3D12GraphicsCommandList* commandList) noexcept;
	void SetMarker(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) noexcept;
}