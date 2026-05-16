#include "PCH.h"

#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <D3D12MemAlloc.h>

static const auto g_d3d12MemoryLogger = Logging::GetOrCreateLogger("RHI.D3D12.Memory");

struct D3D12GpuMemoryAllocator::Impl
{
	D3D12MA::Allocator* allocator = nullptr;

	~Impl() noexcept
	{
		if (allocator != nullptr)
		{
			allocator->Release();
			allocator = nullptr;
		}
	}
};

D3D12GpuMemoryAllocator::D3D12GpuMemoryAllocator(IDXGIAdapter* adapter, ID3D12Device* device) noexcept :
    m_impl(std::make_unique<Impl>())
{
	if (adapter == nullptr || device == nullptr)
	{
		Diagnostics::Fail(g_d3d12MemoryLogger, __FILE__, __LINE__, "D3D12GpuMemoryAllocator requires a valid adapter and device");
	}

	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pAdapter = adapter;
	allocatorDesc.pDevice = device;

	CHECK(D3D12MA::CreateAllocator(&allocatorDesc, &m_impl->allocator));

	if (Environment::GetFlag("SPARKLE_RHI_MEMORY_DIAGNOSTICS"))
	{
		SPDLOG_LOGGER_INFO(g_d3d12MemoryLogger, "D3D12MA allocator initialized");
	}
}

D3D12GpuMemoryAllocator::~D3D12GpuMemoryAllocator() noexcept = default;

bool D3D12GpuMemoryAllocator::IsInitialized() const noexcept
{
	return m_impl != nullptr && m_impl->allocator != nullptr;
}
