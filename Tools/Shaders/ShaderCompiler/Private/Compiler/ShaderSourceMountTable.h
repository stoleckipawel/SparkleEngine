#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class ShaderSourceMountTable final
{
public:
	struct PluginMount final
	{
		std::string Name;
		std::filesystem::path PhysicalRoot;
	};

	ShaderSourceMountTable(
	    std::filesystem::path engineRoot,
	    std::filesystem::path projectRoot,
	    std::span<const PluginMount> pluginMounts = {});

	std::string CanonicalizeVirtualPath(std::string_view path) const;
	std::string ResolveInclude(std::string_view includerPath, std::string_view includePath) const;
	std::filesystem::path ResolvePhysicalPath(std::string_view virtualPath) const;

private:
	struct Mount final
	{
		std::string VirtualRoot;
		std::filesystem::path PhysicalRoot;
	};

	static std::string CanonicalizeMountRoot(std::string_view root);
	static std::filesystem::path CanonicalizePhysicalRoot(const std::filesystem::path& root);
	static std::string FoldAsciiCase(std::string_view value);
	static void ValidatePhysicalPathCase(
	    const std::filesystem::path& root,
	    const std::filesystem::path& relativePath,
	    std::string_view virtualPath);
	static bool IsWithinRoot(const std::filesystem::path& path, const std::filesystem::path& root);
	static bool IsPluginNameValid(std::string_view name) noexcept;
	const Mount& ResolveMount(std::string_view canonicalVirtualPath) const;

	std::vector<Mount> m_mounts;
};
