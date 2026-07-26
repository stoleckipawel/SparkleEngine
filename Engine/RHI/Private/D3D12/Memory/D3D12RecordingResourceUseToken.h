#pragma once

#include <cstdint>

class D3D12RecordingResourceTable;

class D3D12RecordingResourceUseToken final
{
  public:
	constexpr explicit operator bool() const noexcept { return m_value != 0; }

  private:
	friend class D3D12RecordingResourceTable;

	std::uintptr_t m_value = 0;
};
