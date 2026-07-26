#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"
#include "Frame/Core/FrameContext.h"

#include <cassert>
#include <string>
#include <utility>

class FrameGraphPassLabelFormatter final
{
  public:
	static std::string FormatPassEventScopeLabel(FrameGraphPassIndex passIndex, std::string_view passName, EFrameGraphPassKind passKind)
	{
		std::string label{"FrameGraph/"};
		label += FrameGraphPassKindToString(passKind);
		label += "/";
		label += std::to_string(passIndex);
		label += "/";
		label.append(passName.begin(), passName.end());
		return label;
	}

	static std::string FormatPassDiagnosticName(std::string_view passName)
	{
		std::string name{"Renderer.FrameGraph."};
		name.append(passName.begin(), passName.end());
		return name;
	}
};

void FrameGraph::Setup(const FrameContext& frame)
{
	m_compiledPlan.Clear();
	m_compiledPlan.productRoots = m_productRoots;
	m_compiledPlan.passes.reserve(m_passes.size());

	for (std::size_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
	{
		auto& pass = m_passes[passIndex];
		std::vector<PassResourceDeclaration> declarations;
		PassResourceBuilder builder(declarations);
		pass.active = pass.setupCallback(builder, frame);
		FrameGraphResourceContractDiagnostics::ValidatePassDeclarations(pass.name, pass.kind, declarations);
		assert(IsQueuePreferenceCompatible(pass.kind, pass.queuePreference));
		m_compiledPlan.passes.push_back(
		    FrameGraphPassNode{
		        .index = static_cast<FrameGraphPassIndex>(passIndex),
		        .passName = pass.name,
		        .kind = pass.kind,
		        .queuePreference = pass.queuePreference,
		        .diagnosticName = FrameGraphPassLabelFormatter::FormatPassDiagnosticName(pass.name),
		        .eventScopeLabel = FrameGraphPassLabelFormatter::FormatPassEventScopeLabel(static_cast<FrameGraphPassIndex>(passIndex), pass.name, pass.kind),
		        .declarations = std::move(declarations)});
	}
}
