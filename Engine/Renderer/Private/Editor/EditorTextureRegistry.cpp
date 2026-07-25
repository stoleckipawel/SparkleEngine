#include "PCH.h"
#include "Editor/EditorTextureRegistry.h"

#include <limits>

EditorTextureHandle EditorTextureRegistry::PublishViewportTexture(
    std::uint64_t nativeTextureId,
    std::uint64_t viewportGeneration) noexcept
{
	if (nativeTextureId == 0 || viewportGeneration == 0 ||
	    viewportGeneration > (std::numeric_limits<std::uint32_t>::max)())
	{
		RetireViewportTexture();
		return {};
	}

	m_viewportHandle = EditorTextureHandle{
	    .Slot = 1,
	    .Generation = static_cast<std::uint32_t>(viewportGeneration)};
	m_viewportNativeTextureId = nativeTextureId;
	return m_viewportHandle;
}

EditorTextureHandle EditorTextureRegistry::Register(std::uint64_t nativeTextureId) noexcept
{
	if (nativeTextureId == 0)
	{
		return {};
	}

	const EditorTextureHandle candidate = EditorTextureHandle::Unpack(nativeTextureId);
	if (candidate == m_viewportHandle)
	{
		return candidate;
	}
	for (const Binding& binding : m_bindings)
	{
		if (binding.Handle == candidate || binding.NativeTextureId == nativeTextureId)
		{
			return binding.Handle;
		}
	}

	if (m_bindings.size() >= (std::numeric_limits<std::uint32_t>::max)() - 2)
	{
		return {};
	}
	const EditorTextureHandle handle{
	    .Slot = static_cast<std::uint32_t>(m_bindings.size() + 2),
	    .Generation = 1};
	m_bindings.push_back(Binding{.Handle = handle, .NativeTextureId = nativeTextureId});
	return handle;
}

std::uint64_t EditorTextureRegistry::Resolve(std::uint64_t packedHandle) const noexcept
{
	const EditorTextureHandle handle = EditorTextureHandle::Unpack(packedHandle);
	if (handle == m_viewportHandle)
	{
		return m_viewportNativeTextureId;
	}
	for (const Binding& binding : m_bindings)
	{
		if (binding.Handle == handle)
		{
			return binding.NativeTextureId;
		}
	}
	return 0;
}

void EditorTextureRegistry::RetireViewportTexture() noexcept
{
	m_viewportHandle = {};
	m_viewportNativeTextureId = 0;
}
