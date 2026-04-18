#pragma once

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <ObjIdl.h>
#include <Unknwn.h>

#include <dxcapi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DxcContext
{
  public:
	DxcContext();
	~DxcContext() = default;

	DxcContext(const DxcContext&) = delete;
	DxcContext& operator=(const DxcContext&) = delete;
	DxcContext(DxcContext&&) = delete;
	DxcContext& operator=(DxcContext&&) = delete;

	bool IsValid() const noexcept { return m_compiler != nullptr && m_utils != nullptr; }

	IDxcCompiler3* GetCompiler() const noexcept { return m_compiler.Get(); }
	IDxcUtils* GetUtils() const noexcept { return m_utils.Get(); }

	ComPtr<IDxcIncludeHandler> CreateIncludeHandler() const;

  private:
	ComPtr<IDxcCompiler3> m_compiler;
	ComPtr<IDxcUtils> m_utils;
};

DxcContext& GetDxcContext();