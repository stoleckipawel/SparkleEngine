#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"
#include "Frame/FrameContext.h"

#include <string>
#include <utility>

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
}  // namespace

void FrameGraph::Setup(const FrameContext& frame)
{
	m_compiledPlan.Clear();
	m_compiledPlan.passes.reserve(m_passes.size());

	for (std::size_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
	{
		auto& pass = m_passes[passIndex];
		std::vector<PassResourceDeclaration> declarations;
		PassResourceDeclarationSink declarationSink(declarations);
		PassResourceBuilder builder(declarationSink);
		pass.setupCallback(builder, frame);
		FrameGraphResourceContractDiagnostics::ValidatePassDeclarations(pass.name, pass.flags, declarations);
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
