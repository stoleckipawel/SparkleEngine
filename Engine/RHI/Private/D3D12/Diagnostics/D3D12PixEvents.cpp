#include "PCH.h"

#include "D3D12/Diagnostics/D3D12PixEvents.h"

namespace D3D12PixEvents
{
	struct Runtime final
	{
		using BeginEventOnCommandListFn = void(WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, PCSTR formatString);
		using EndEventOnCommandListFn = void(WINAPI*)(ID3D12GraphicsCommandList* commandList);
		using SetMarkerOnCommandListFn = void(WINAPI*)(ID3D12GraphicsCommandList* commandList, UINT64 color, PCSTR formatString);

		Runtime() noexcept
		{
			module = LoadLibraryW(L"WinPixEventRuntime.dll");
			if (module == nullptr)
			{
				return;
			}

			beginEventOnCommandList = reinterpret_cast<BeginEventOnCommandListFn>(GetProcAddress(module, "PIXBeginEventOnCommandList"));
			endEventOnCommandList = reinterpret_cast<EndEventOnCommandListFn>(GetProcAddress(module, "PIXEndEventOnCommandList"));
			setMarkerOnCommandList = reinterpret_cast<SetMarkerOnCommandListFn>(GetProcAddress(module, "PIXSetMarkerOnCommandList"));
		}

		bool IsAvailable() const noexcept
		{
			return module != nullptr && beginEventOnCommandList != nullptr && endEventOnCommandList != nullptr &&
			       setMarkerOnCommandList != nullptr;
		}

		HMODULE module = nullptr;
		BeginEventOnCommandListFn beginEventOnCommandList = nullptr;
		EndEventOnCommandListFn endEventOnCommandList = nullptr;
		SetMarkerOnCommandListFn setMarkerOnCommandList = nullptr;
	};

	const Runtime& GetRuntime() noexcept
	{
		static const Runtime runtime;
		return runtime;
	}

	bool IsAvailable() noexcept
	{
		return GetRuntime().IsAvailable();
	}

	UINT64 ToColor(RhiDiagnosticLabelColor color) noexcept
	{
		return (static_cast<UINT64>(0xFFu) << 24u) | (static_cast<UINT64>(color.Red) << 16u) | (static_cast<UINT64>(color.Green) << 8u) |
		       static_cast<UINT64>(color.Blue);
	}

	void BeginEvent(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) noexcept
	{
		const Runtime& runtime = GetRuntime();
		if (runtime.IsAvailable() && commandList != nullptr && label != nullptr)
		{
			runtime.beginEventOnCommandList(commandList, color, label);
		}
	}

	void EndEvent(ID3D12GraphicsCommandList* commandList) noexcept
	{
		const Runtime& runtime = GetRuntime();
		if (runtime.IsAvailable() && commandList != nullptr)
		{
			runtime.endEventOnCommandList(commandList);
		}
	}

	void SetMarker(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* label) noexcept
	{
		const Runtime& runtime = GetRuntime();
		if (runtime.IsAvailable() && commandList != nullptr && label != nullptr)
		{
			runtime.setMarkerOnCommandList(commandList, color, label);
		}
	}
}