#pragma once

#include "Presentation/RhiPresentationService.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

template <typename Owner> class RhiPresentationServiceAdapter final : public RhiPresentationService
{
public:
	explicit RhiPresentationServiceAdapter(Owner& owner) noexcept :
	    m_owner(owner)
	{
	}

	RhiViewport GetBackBufferViewport() const noexcept override { return m_owner.GetBackBufferViewport(); }
	RhiRect GetBackBufferScissorRect() const noexcept override { return m_owner.GetBackBufferScissorRect(); }
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept override { return m_owner.GetBackBufferRenderTargetView(); }
	RhiResourceHandle GetBackBufferResource() const noexcept override { return m_owner.GetBackBufferResource(); }
	void BeginPresentRenderPass(const float clearColor[4]) noexcept override
	{
		Begin();
		m_owner.BeginPresentRenderPass(clearColor);
	}
	void BeginPresentOverlayPass() noexcept override
	{
		Begin();
		m_owner.BeginPresentOverlayPass();
	}
	void EndPresentRenderPass() noexcept override
	{
		if (!m_rendering)
		{
			Fail("Present rendering ended without a matching begin.");
		}
		m_owner.EndPresentRenderPass();
		m_rendering = false;
	}
	PixelFormat GetPresentColorFormat() const noexcept override { return m_owner.GetPresentColorFormat(); }

private:
	void Begin() noexcept
	{
		if (m_rendering)
		{
			Fail("Present rendering began while another present render pass was active.");
		}
		m_rendering = true;
	}

	[[noreturn]] static void Fail(const char* message) noexcept
	{
		static const auto logger = Logging::GetOrCreateLogger("RHI.Presentation");
		Diagnostics::Fatal(logger, __FILE__, __LINE__, message);
	}

	Owner& m_owner;
	bool m_rendering = false;
};
