#pragma once

#include <cstddef>

// Lightweight non-owning view of cooked shader bytecode. Lives in RHI public
// because the cooked code library returns it; the offline compile result
// type that previously co-located here moved out of RHI public into the
// offline shader-cooking tool's public surface.
struct ShaderBytecode
{
	const void* Data = nullptr;
	std::size_t Size = 0;

	bool IsValid() const noexcept { return Data != nullptr && Size > 0; }
	explicit operator bool() const noexcept { return IsValid(); }
};
