#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

class ShaderStageNames final
{
  public:
	static std::optional<ShaderStage> TryParse(std::string_view name) noexcept;
	static std::string_view ToString(ShaderStage stage) noexcept;
	static std::string FormatMask(ShaderStageMask mask);

  private:
	struct Entry final
	{
		ShaderStage stage;
		ShaderStageMask mask;
		std::string_view name;
	};

	static constexpr std::size_t kEntryCount = 6;
	static const std::array<Entry, kEntryCount> kTable;
};
