#include "PCH.h"

#include "Compiler/DxcContext.h"

DxcContext::DxcContext()
{
	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		LOG_FATAL("Failed to create DXC compiler instance");
		return;
	}

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		LOG_FATAL("Failed to create DXC utils instance");
		m_compiler.Reset();
		return;
	}
}

ComPtr<IDxcIncludeHandler> DxcContext::CreateIncludeHandler() const
{
	ComPtr<IDxcIncludeHandler> handler;
	if (m_utils)
	{
		m_utils->CreateDefaultIncludeHandler(handler.ReleaseAndGetAddressOf());
	}
	return handler;
}

DxcContext& GetDxcContext()
{
	static DxcContext sContext;
	return sContext;
}