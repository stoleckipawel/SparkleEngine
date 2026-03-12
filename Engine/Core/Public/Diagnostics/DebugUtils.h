#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace DebugUtils
{
	// Set debug name for a raw D3D12 object. No-op in release builds.
	inline void SetDebugName(ID3D12Object* object, const wchar_t* name) noexcept
	{
#if defined(_DEBUG)
		if (object && name)
		{
			object->SetName(name);
		}
#endif
	}

	// Overload for ComPtr
	template <typename T> inline void SetDebugName(const ComPtr<T>& object, const wchar_t* name) noexcept
	{
		SetDebugName(object.Get(), name);
	}

	// Convenience overload for std::wstring_view
	inline void SetDebugName(ID3D12Object* object, const std::wstring_view name) noexcept
	{
#if defined(_DEBUG)
		if (object && !name.empty())
			object->SetName(name.data());
#endif
	}

}  // namespace DebugUtils
