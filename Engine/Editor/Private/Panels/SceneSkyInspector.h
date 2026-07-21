#pragma once

#include <string>
#include <cstdint>
#include <optional>

class EditorTransactionManager;
struct SkyEnvironment;

class SceneSkyInspector final
{
  public:
	static void Build(const std::optional<SkyEnvironment>&, EditorTransactionManager&, std::uint64_t, const std::string&) noexcept;
};
