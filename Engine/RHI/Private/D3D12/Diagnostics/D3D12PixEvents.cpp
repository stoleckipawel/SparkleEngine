#include "PCH.h"

#include "D3D12/Diagnostics/D3D12PixEvents.h"

#include <pix3.h>

namespace D3D12PixEvents
{
	UINT64 ToColor(RhiDiagnosticLabelColor color) noexcept
	{
		return PIX_COLOR(color.Red, color.Green, color.Blue);
	}

	void BeginEvent(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) noexcept
	{
		PIXBeginEvent(commandList, color, "%s", label);
	}

	void EndEvent(ID3D12GraphicsCommandList* commandList) noexcept
	{
		PIXEndEvent(commandList);
	}

	void SetMarker(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) noexcept
	{
		PIXSetMarker(commandList, color, "%s", label);
	}
}
