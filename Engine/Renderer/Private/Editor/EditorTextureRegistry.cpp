#include "PCH.h"
#include "Editor/EditorTextureRegistry.h"

#include <limits>

EditorTextureHandle EditorTextureRegistry::PublishViewportTexture(std::uint64_t nativeTextureId, std::uint64_t viewportGeneration) noexcept
{
	const EditorTextureHandle handle = EditorTextureHandle::Viewport(viewportGeneration);
	if (nativeTextureId == 0 || !handle)
	{
		RetireViewportTexture();
		return {};
	}

	m_viewportHandle = handle;
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

	if (m_bindings.size() >= (std::numeric_limits<std::uint32_t>::max)() - 3)
	{
		return {};
	}
	const EditorTextureHandle handle{.Slot = static_cast<std::uint32_t>(m_bindings.size() + 3), .Generation = 1};
	m_bindings.push_back(Binding{.Handle = handle, .NativeTextureId = nativeTextureId});
	return handle;
}

std::uint64_t EditorTextureRegistry::Resolve(EditorTextureHandle handle) const noexcept
{
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
