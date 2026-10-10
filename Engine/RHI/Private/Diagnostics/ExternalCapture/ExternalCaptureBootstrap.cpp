#include "PCH.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureAdapter.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <Windows.h>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_externalCaptureBootstrapLogger, "RHI.ExternalCapture.Bootstrap");

static bool HasInjectedCaptureHooks() noexcept
{
	return GetModuleHandleW(L"WinPixGpuCapturer.dll") != nullptr || GetModuleHandleW(L"renderdoc.dll") != nullptr || GetModuleHandleW(L"ngfx-capture-injection.dll") != nullptr
	    || GetModuleHandleW(L"ngfx-capture-interception.dll") != nullptr;
}

static bool ValidateInjectedCaptureActivity(ExternalCaptureProvider provider, std::string& error)
{
	if (GetModuleHandleW(L"WarpVizTarget.dll") != nullptr || GetModuleHandleW(L"ToolsInjection64.dll") != nullptr)
	{
		error = "A trace/system activity is already injected. Graphics Capture requires a separate process.";
		return false;
	}

	const bool pixLoaded = GetModuleHandleW(L"WinPixGpuCapturer.dll") != nullptr;
	const bool renderDocLoaded = GetModuleHandleW(L"renderdoc.dll") != nullptr;
	const bool nsightLoaded = GetModuleHandleW(L"ngfx-capture-interception.dll") != nullptr;
	if ((pixLoaded && provider != ExternalCaptureProvider::Pix) || (renderDocLoaded && provider != ExternalCaptureProvider::RenderDoc)
	    || (nsightLoaded && provider != ExternalCaptureProvider::NsightGraphics))
	{
		error = "Another capture provider is injected. Restart with one provider; hook combinations are unverified.";
		return false;
	}

	return true;
}

static std::unique_ptr<ExternalCaptureAdapter> BootstrapCaptureProvider(ERhiBackendApi api, ExternalCaptureProvider provider, std::string& error)
{
	if (provider == ExternalCaptureProvider::RenderDoc)
	{
		return CreateRenderDocCaptureAdapter(error);
	}

	if (api != ERhiBackendApi::D3D12)
	{
		error = "This provider's in-editor capture is currently available only on D3D12.";
		return {};
	}

#if SPARKLE_RHI_WITH_D3D12
	switch (provider)
	{
		case ExternalCaptureProvider::Pix:
			return CreatePixCaptureAdapter(error);
		case ExternalCaptureProvider::NsightGraphics:
			return CreateNsightCaptureAdapter(error);
		default:
			break;
	}
#endif

	return {};
}

std::unique_ptr<ExternalCaptureAdapter> CreateExternalCaptureAdapter(ERhiBackendApi api, ExternalCaptureProvider provider, std::string& error)
{
	if (!ValidateInjectedCaptureActivity(provider, error))
	{
		return {};
	}

	auto adapter = BootstrapCaptureProvider(api, provider, error);
	if (adapter == nullptr && HasInjectedCaptureHooks())
	{
		Diagnostics::Fatal(g_externalCaptureBootstrapLogger, __FILE__, __LINE__, error);
	}

	return adapter;
}
