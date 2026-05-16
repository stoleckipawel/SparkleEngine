#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Frame/FrameContext.h"

#include <cassert>
#include <string>
#include <utility>

static const auto g_frameGraphDeclarationLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace
{
	std::string FormatPassDisplayLabel(FrameGraphPassIndex passIndex, std::string_view passName, EFrameGraphPassFlags flags)
	{
		std::string label{"["};
		label += FrameGraphPassKindToString(flags);
		label += " #";
		label += std::to_string(passIndex);
		label += "] ";
		label.append(passName.begin(), passName.end());
		return label;
	}

	std::string FormatPassEventScopeLabel(FrameGraphPassIndex passIndex, std::string_view passName, EFrameGraphPassFlags flags)
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

void FrameGraph::Setup(const FrameContext& frame)
{
	ReleaseExternalViewDescriptors();
	m_compiledPlan.Clear();
	m_compiledPlan.passes.reserve(m_passes.size());

	for (std::size_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
	{
		auto& pass = m_passes[passIndex];
		std::vector<PassResourceDeclaration> declarations;
		PassResourceDeclarationSink declarationSink(declarations);
		PassResourceBuilder builder(declarationSink);
		pass.setupCallback(builder, frame);
		ValidatePassDeclarations(pass.name, pass.flags, declarations);
		m_compiledPlan.passes.push_back(
		    FrameGraphPassNode{
		        .index = static_cast<FrameGraphPassIndex>(passIndex),
		        .passName = pass.name,
		        .flags = pass.flags,
		        .passKind = GetFrameGraphPassKind(pass.flags),
		        .diagnosticName = FormatPassDiagnosticName(pass.name),
		        .displayLabel = FormatPassDisplayLabel(static_cast<FrameGraphPassIndex>(passIndex), pass.name, pass.flags),
		        .eventScopeLabel = FormatPassEventScopeLabel(static_cast<FrameGraphPassIndex>(passIndex), pass.name, pass.flags),
		        .declarations = std::move(declarations)});
	}
}
