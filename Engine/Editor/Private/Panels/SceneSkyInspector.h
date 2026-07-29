#pragma once

#include <string>
#include <cstdint>
#include <optional>

class EditorTransactionHistory;
struct SkyEnvironment;

class SceneSkyInspector final
{
  public:
	static void Build(const std::optional<SkyEnvironment>&, EditorTransactionHistory&, std::uint64_t, const std::string&) noexcept;
};
