#include "PCH.h"
#include "UI/UiTextureRegistry.h"

#include <limits>

UiTextureHandle UiTextureRegistry::PublishViewportTexture(std::uint64_t nativeTextureId, std::uint64_t viewportGeneration) noexcept
{
	const UiTextureHandle handle = UiTextureHandle::Viewport(viewportGeneration);
	if (nativeTextureId == 0 || !handle)
	{
		RetireViewportTexture();
		return {};
	}

	m_viewportHandle = handle;
	m_viewportNativeTextureId = nativeTextureId;
	return m_viewportHandle;
}

UiTextureHandle UiTextureRegistry::Register(std::uint64_t nativeTextureId) noexcept
{
	if (nativeTextureId == 0)
	{
		return {};
	}

	const UiTextureHandle candidate = UiTextureHandle::Unpack(nativeTextureId);
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
	const UiTextureHandle handle{.Slot = static_cast<std::uint32_t>(m_bindings.size() + 3), .Generation = 1};
	m_bindings.push_back(Binding{.Handle = handle, .NativeTextureId = nativeTextureId});
	return handle;
}

std::uint64_t UiTextureRegistry::Resolve(UiTextureHandle handle) const noexcept
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

void UiTextureRegistry::RetireViewportTexture() noexcept
{
	m_viewportHandle = {};
	m_viewportNativeTextureId = 0;
}
