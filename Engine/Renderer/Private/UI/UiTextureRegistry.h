#pragma once

#include "Renderer/Public/UI/UiTextureHandle.h"

#include <cstdint>
#include <vector>

class UiTextureRegistry final
{
public:
	UiTextureHandle PublishViewportTexture(std::uint64_t nativeTextureId, std::uint64_t viewportGeneration) noexcept;
	UiTextureHandle Register(std::uint64_t nativeTextureId) noexcept;
	std::uint64_t Resolve(UiTextureHandle handle) const noexcept;
	void RetireViewportTexture() noexcept;

private:
	UiTextureHandle m_viewportHandle;
	std::uint64_t m_viewportNativeTextureId = 0;
	struct Binding final
	{
		UiTextureHandle Handle;
		std::uint64_t NativeTextureId = 0;
	};
	std::vector<Binding> m_bindings;
};
