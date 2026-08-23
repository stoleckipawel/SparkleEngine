#include "PCH.h"

#include "Compiler/ShaderSourceMountTable.h"

#include "Compiler/ShaderCompilerPaths.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"

#include <algorithm>
#include <cctype>
#include <format>

std::string ShaderSourceMountTable::FoldAsciiCase(std::string_view value)
{
	std::string folded(value);
	std::ranges::transform(
	    folded,
	    folded.begin(),
	    [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return folded;
}

void ShaderSourceMountTable::ValidatePhysicalPathCase(
    const std::filesystem::path& root,
    const std::filesystem::path& relativePath,
    std::string_view virtualPath)
{
	std::filesystem::path parent = root;
	for (const std::filesystem::path& segment : relativePath)
	{
		const std::string expected = segment.generic_string();
		bool exactMatch = false;
		bool caseCollision = false;
		std::error_code errorCode;
		for (std::filesystem::directory_iterator entry(parent, errorCode), end; !errorCode && entry != end; entry.increment(errorCode))
		{
			const std::string candidate = entry->path().filename().generic_string();
			if (candidate == expected)
			{
				exactMatch = true;
				continue;
			}
			caseCollision = caseCollision || FoldAsciiCase(candidate) == FoldAsciiCase(expected);
		}
		if (caseCollision)
		{
			throw Diagnostics::Error(
			    exactMatch
			        ? std::format(
			              "Shader source path '{}' collides with another authored file under the virtual-path case policy.",
			              virtualPath)
			        : std::format("Shader source path '{}' does not match the authored file casing.", virtualPath));
		}
		if (!exactMatch)
		{
			return;
		}
		parent /= segment;
	}
}

bool ShaderSourceMountTable::IsWithinRoot(const std::filesystem::path& path, const std::filesystem::path& root)
{
	const std::wstring pathKey = Paths::MakePathKey(path);
	std::wstring rootKey = Paths::MakePathKey(root);
	if (!rootKey.empty() && rootKey.back() != L'/')
	{
		rootKey.push_back(L'/');
	}
	return pathKey == Paths::MakePathKey(root) || pathKey.starts_with(rootKey);
}

ShaderSourceMountTable::ShaderSourceMountTable(
    std::filesystem::path engineRoot,
    std::filesystem::path projectRoot,
    std::span<const PluginMount> pluginMounts)
{
	auto addMount = [this](std::string virtualRoot, const std::filesystem::path& physicalRoot)
	{
		if (physicalRoot.empty())
		{
			return;
		}

		Mount mount{
		    .VirtualRoot = CanonicalizeMountRoot(virtualRoot),
		    .PhysicalRoot = CanonicalizePhysicalRoot(physicalRoot)};
		for (const Mount& existing : m_mounts)
		{
			if (FoldAsciiCase(existing.VirtualRoot) == FoldAsciiCase(mount.VirtualRoot))
			{
				throw Diagnostics::Error(std::format(
				    "Shader source mounts '{}' and '{}' collide under the virtual-path case policy.",
				    existing.VirtualRoot,
				    mount.VirtualRoot));
			}
			if (IsWithinRoot(existing.PhysicalRoot, mount.PhysicalRoot) || IsWithinRoot(mount.PhysicalRoot, existing.PhysicalRoot))
			{
				throw Diagnostics::Error(std::format(
				    "Shader source mounts '{}' and '{}' have overlapping physical ownership.",
				    existing.VirtualRoot,
				    mount.VirtualRoot));
			}
		}
		m_mounts.push_back(std::move(mount));
	};

	addMount("/Engine", engineRoot);
	addMount("/Project", projectRoot);
	for (const PluginMount& plugin : pluginMounts)
	{
		if (!IsPluginNameValid(plugin.Name))
		{
			throw Diagnostics::Error(std::format("Invalid shader plugin mount name '{}'.", plugin.Name));
		}
		addMount(std::format("/Plugin/{}", plugin.Name), plugin.PhysicalRoot);
	}

	if (std::ranges::none_of(m_mounts, [](const Mount& mount) { return mount.VirtualRoot == "/Engine"; }))
	{
		throw Diagnostics::Error("The /Engine shader source mount requires a physical root.");
	}

	std::ranges::sort(
	    m_mounts,
	    [](const Mount& lhs, const Mount& rhs) { return lhs.VirtualRoot.size() > rhs.VirtualRoot.size(); });
}

std::string ShaderSourceMountTable::CanonicalizeVirtualPath(std::string_view path) const
{
	if (path.empty() || path.front() != '/' || path.find('\\') != std::string_view::npos || path.find(':') != std::string_view::npos)
	{
		throw Diagnostics::Error(std::format("Shader source path '{}' is not a canonical virtual path.", path));
	}

	std::string canonical(path);
	if (canonical.size() > 1u && canonical.back() == '/')
	{
		canonical.pop_back();
	}
	if (canonical.find("//") != std::string::npos)
	{
		throw Diagnostics::Error(std::format("Shader source path '{}' contains an empty segment.", path));
	}

	std::size_t segmentStart = 1u;
	while (segmentStart <= canonical.size())
	{
		const std::size_t segmentEnd = canonical.find('/', segmentStart);
		const std::size_t segmentLength =
		    (segmentEnd == std::string::npos ? canonical.size() : segmentEnd) - segmentStart;
		const std::string_view segment(canonical.data() + segmentStart, segmentLength);
		if (segment.empty() || segment == "." || segment == "..")
		{
			throw Diagnostics::Error(std::format("Shader source path '{}' contains a forbidden traversal segment.", path));
		}
		if (segmentEnd == std::string::npos)
		{
			break;
		}
		segmentStart = segmentEnd + 1u;
	}

	(void) ResolveMount(canonical);
	return canonical;
}

std::string ShaderSourceMountTable::ResolveInclude(std::string_view includerPath, std::string_view includePath) const
{
	if (includePath.empty())
	{
		throw Diagnostics::Error("Shader include path is empty.");
	}
	if (includePath.front() == '/')
	{
		return CanonicalizeVirtualPath(includePath);
	}
	if (includePath.find('\\') != std::string_view::npos || includePath.find(':') != std::string_view::npos)
	{
		throw Diagnostics::Error(std::format("Shader include '{}' is not virtual or relative.", includePath));
	}

	const std::string includer = CanonicalizeVirtualPath(includerPath);
	const std::size_t separator = includer.find_last_of('/');
	return CanonicalizeVirtualPath(includer.substr(0, separator + 1u) + std::string(includePath));
}

std::filesystem::path ShaderSourceMountTable::ResolvePhysicalPath(std::string_view virtualPath) const
{
	const std::string canonical = CanonicalizeVirtualPath(virtualPath);
	const Mount& mount = ResolveMount(canonical);
	std::string_view relative = std::string_view(canonical).substr(mount.VirtualRoot.size());
	if (!relative.empty() && relative.front() == '/')
	{
		relative.remove_prefix(1u);
	}
	ValidatePhysicalPathCase(mount.PhysicalRoot, std::filesystem::path(relative), canonical);
	const std::filesystem::path physical = ShaderCompilerPaths::CanonicalizeForCompiler(mount.PhysicalRoot / std::filesystem::path(relative));
	if (!IsWithinRoot(physical, mount.PhysicalRoot))
	{
		throw Diagnostics::Error(std::format("Shader source path '{}' escapes mount '{}'.", canonical, mount.VirtualRoot));
	}
	return physical;
}

std::string ShaderSourceMountTable::CanonicalizeMountRoot(std::string_view root)
{
	if (root.empty() || root.front() != '/' || root.back() == '/' || root.find('\\') != std::string_view::npos ||
	    root.find("//") != std::string_view::npos)
	{
		throw Diagnostics::Error(std::format("Shader source mount '{}' is not canonical.", root));
	}
	return std::string(root);
}

std::filesystem::path ShaderSourceMountTable::CanonicalizePhysicalRoot(const std::filesystem::path& root)
{
	return ShaderCompilerPaths::CanonicalizeForCompiler(root);
}

bool ShaderSourceMountTable::IsPluginNameValid(std::string_view name) noexcept
{
	return !name.empty() && std::ranges::all_of(
	                            name,
	                            [](unsigned char value) { return std::isalnum(value) != 0 || value == '_' || value == '-'; });
}

const ShaderSourceMountTable::Mount& ShaderSourceMountTable::ResolveMount(std::string_view canonicalVirtualPath) const
{
	for (const Mount& mount : m_mounts)
	{
		if (canonicalVirtualPath == mount.VirtualRoot ||
		    (canonicalVirtualPath.starts_with(mount.VirtualRoot) && canonicalVirtualPath.size() > mount.VirtualRoot.size() &&
		     canonicalVirtualPath[mount.VirtualRoot.size()] == '/'))
		{
			return mount;
		}
	}
	throw Diagnostics::Error(std::format("Shader source path '{}' has no registered virtual mount.", canonicalVirtualPath));
}
