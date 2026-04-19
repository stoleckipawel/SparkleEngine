#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Frame/FrameContext.h"

#include <cassert>
#include <string>

static const auto g_frameGraphDeclarationLogger = Engine::Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace
{
	std::string FormatPassDisplayLabel(FrameGraph::PassIndex passIndex, std::string_view passName)
	{
		return std::string{"#"} + std::to_string(passIndex) + ":" + std::string(passName);
	}

	std::string FormatPassEventScopeLabel(FrameGraph::PassIndex passIndex, std::string_view passName, EFrameGraphPassFlags flags)
	{
		std::string label{"FrameGraph/"};
		label += FrameGraphPassKindToString(flags);
		label += "/";
		label += std::to_string(passIndex);
		label += "/";
		label.append(passName.begin(), passName.end());
		return label;
	}

	std::string FormatPassDiagnosticName(std::string_view passName)
	{
		std::string name{"Renderer.FrameGraph."};
		name.append(passName.begin(), passName.end());
		return name;
	}

	void ValidatePassDeclarations(
	    std::string_view passName,
	    EFrameGraphPassFlags flags,
	    const std::vector<PassResourceDeclaration>& declarations) noexcept
	{
		assert(HasExactlyOnePassKind(flags));

		for (const PassResourceDeclaration& declaration : declarations)
		{
			if (ReadsFromUsage(declaration.usage) || WritesToUsage(declaration.usage))
			{
				continue;
			}

			std::string message{"FrameGraph pass '"};
			message.append(passName.begin(), passName.end());
			if (!declaration.label.empty())
			{
				message += "' parameter '";
				message += declaration.label;
			}
			message += "' uses unsupported resource usage ";
			message += ResourceUsageToString(declaration.usage);
			message += ".";
			SPDLOG_LOGGER_WARN(g_frameGraphDeclarationLogger, "{}", message);
			assert(false);
		}
	}
}  // namespace

void FrameGraph::BeginPassSetup() noexcept
{
	m_isSettingUpPass = true;
	m_activePassDeclarations.clear();
}

void FrameGraph::EndPassSetup() noexcept
{
	m_isSettingUpPass = false;
}

void FrameGraph::RecordDeclaration(PassResourceDeclaration declaration) noexcept
{
	m_activePassDeclarations.push_back(declaration);
}

void FrameGraph::Setup(const FrameContext& frame)
{
	ReleaseExternalViewDescriptors();
	m_compiledPlan.Clear();
	m_compiledPlan.passes.reserve(m_passes.size());

	for (std::size_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
	{
		auto& pass = m_passes[passIndex];
		PassBuilder builder(*this);
		BeginPassSetup();
		pass.setupCallback(builder, frame);
		EndPassSetup();
		ValidatePassDeclarations(pass.name, pass.flags, m_activePassDeclarations);
		m_compiledPlan.passes.push_back(
		    CompilePassRecord{
		        .index = static_cast<PassIndex>(passIndex),
		        .passName = pass.name,
		        .flags = pass.flags,
		        .passKind = GetFrameGraphPassKind(pass.flags),
		        .diagnosticName = FormatPassDiagnosticName(pass.name),
		        .displayLabel = FormatPassDisplayLabel(static_cast<PassIndex>(passIndex), pass.name),
		        .eventScopeLabel = FormatPassEventScopeLabel(static_cast<PassIndex>(passIndex), pass.name, pass.flags),
		        .declarations = m_activePassDeclarations});
	}

	m_activePassDeclarations.clear();
}

ResourceHandle FrameGraph::Read(ResourceHandle handle, ResourceUsage usage) noexcept
{
	return Read(handle, usage, {});
}

ResourceHandle FrameGraph::Read(ResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_isSettingUpPass);
	assert(IsReadOnlyUsage(usage));
	RecordDeclaration(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
	return handle;
}

ResourceHandle FrameGraph::Write(ResourceHandle handle, ResourceUsage usage) noexcept
{
	return Write(handle, usage, {});
}

ResourceHandle FrameGraph::Write(ResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_isSettingUpPass);
	assert(IsWriteOnlyUsage(usage));
	RecordDeclaration(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
	return handle;
}

ResourceHandle FrameGraph::Use(ResourceHandle handle, ResourceUsage usage) noexcept
{
	return Use(handle, usage, {});
}

ResourceHandle FrameGraph::Use(ResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_isSettingUpPass);
	assert(IsReadWriteUsage(usage));
	RecordDeclaration(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
	return handle;
}