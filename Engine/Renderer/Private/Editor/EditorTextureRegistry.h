#pragma once

#include "Renderer/Public/Editor/EditorTextureHandle.h"

#include <cstdint>
#include <vector>

class EditorTextureRegistry final
{
  public:
	void PublishFontTexture(std::uint64_t nativeTextureId) noexcept;
	EditorTextureHandle PublishViewportTexture(
	    std::uint64_t nativeTextureId,
	    std::uint64_t viewportGeneration) noexcept;
	EditorTextureHandle Register(std::uint64_t nativeTextureId) noexcept;
	std::uint64_t Resolve(EditorTextureHandle handle) const noexcept;
	void RetireViewportTexture() noexcept;

  private:
	std::uint64_t m_fontNativeTextureId = 0;
	EditorTextureHandle m_viewportHandle;
	std::uint64_t m_viewportNativeTextureId = 0;
	struct Binding final
	{
		EditorTextureHandle Handle;
		std::uint64_t NativeTextureId = 0;
	};
	std::vector<Binding> m_bindings;
};
