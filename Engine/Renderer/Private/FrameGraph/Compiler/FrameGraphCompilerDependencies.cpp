#include "PCH.h"
#include "FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	void AddRawDependency(std::vector<FrameGraph::PassIndex>& dependsOn, FrameGraph::PassIndex dependency) noexcept
	{
		if (dependency == FrameGraph::INVALID_PASS_INDEX)
		{
			return;
		}

		dependsOn.push_back(dependency);
	}

	void RegisterVersionReader(FrameGraph::ResourceVersion& version, FrameGraph::PassIndex readerPass) noexcept
	{
		if (readerPass == FrameGraph::INVALID_PASS_INDEX)
		{
			return;
		}

		const auto it = std::find(version.readerPasses.begin(), version.readerPasses.end(), readerPass);
		if (it == version.readerPasses.end())
		{
			version.readerPasses.push_back(readerPass);
		}
	}
}

void FrameGraphCompiler::BuildResourceVersionGraph() noexcept
{
	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		BuildPassResourceVersionDependencies(passRecord);
	}
}

void FrameGraphCompiler::FinalizePassDependencies() noexcept
{
	DeduplicatePassDependencies();
	CullDeadPasses();
	BuildPassSuccessorsAndInDegrees();
	BuildTopologicalExecutionOrder();
	ValidateExecutionOrder();
}

void FrameGraphCompiler::DeduplicatePassDependencies() noexcept
{
	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		std::vector<PassIndex> deduplicated;
		deduplicated.reserve(passRecord.dependsOn.size());

		for (const PassIndex dependency : passRecord.dependsOn)
		{
			if (dependency == FrameGraph::INVALID_PASS_INDEX || dependency == passRecord.index)
			{
				continue;
			}

			const auto it = std::find(deduplicated.begin(), deduplicated.end(), dependency);
			if (it == deduplicated.end())
			{
				deduplicated.push_back(dependency);
			}
		}

		passRecord.dependsOn = std::move(deduplicated);
	}
}

void FrameGraphCompiler::CullDeadPasses() noexcept
{
	std::vector<PassIndex> rootPasses;
	rootPasses.reserve(m_plan.passes.size());

	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		passRecord.alive = false;
		if (GetRootPassReason(passRecord) != nullptr)
		{
			rootPasses.push_back(passRecord.index);
		}
	}

	assert(m_plan.passes.empty() || !rootPasses.empty());

	for (const PassIndex rootPass : rootPasses)
	{
		MarkPassAliveRecursive(rootPass);
	}

}

void FrameGraphCompiler::MarkPassAliveRecursive(PassIndex passIndex) noexcept
{
	assert(passIndex < m_plan.passes.size());
	CompilePassRecord& passRecord = m_plan.passes[passIndex];
	if (passRecord.alive)
	{
		return;
	}

	passRecord.alive = true;
	for (const PassIndex dependency : passRecord.dependsOn)
	{
		if (dependency == FrameGraph::INVALID_PASS_INDEX)
		{
			continue;
		}

		MarkPassAliveRecursive(dependency);
	}
}

bool FrameGraphCompiler::IsRootPass(const CompilePassRecord& passRecord) const noexcept
{
	return GetRootPassReason(passRecord) != nullptr;
}

const char* FrameGraphCompiler::GetRootPassReason(const CompilePassRecord& passRecord) const noexcept
{
	if (WritesBackBuffer(passRecord))
	{
		return "backbuffer-output";
	}

	return nullptr;
}






















bool FrameGraphCompiler::WritesBackBuffer(const CompilePassRecord& passRecord) const noexcept
{
	for (const PassResourceDeclaration& declaration : passRecord.declarations)
	{
		if (!declaration.handle.IsValid() || !IsWriteOnlyUsage(declaration.usage))
		{
			continue;
		}

		const CompileResourceEntry& resource = GetCompiledResourceEntry(declaration.handle);
		if (resource.kind == FrameGraphResourceKind::BackBuffer)
		{
			return true;
		}
	}

	return false;
}

void FrameGraphCompiler::BuildPassSuccessorsAndInDegrees() noexcept
{
	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		passRecord.successors.clear();
		passRecord.inDegree = 0;
	}

	for (CompilePassRecord& passRecord : m_plan.passes)
	{
		if (!passRecord.alive)
		{
			continue;
		}

		for (const PassIndex dependency : passRecord.dependsOn)
		{
			assert(dependency < m_plan.passes.size());
			CompilePassRecord& dependencyPass = m_plan.passes[dependency];
			assert(dependencyPass.alive);
			dependencyPass.successors.push_back(passRecord.index);
			++passRecord.inDegree;
		}
	}
}

void FrameGraphCompiler::BuildTopologicalExecutionOrder() noexcept
{
	m_plan.executionOrder.clear();
	m_plan.executionOrder.reserve(m_plan.passes.size());

	std::vector<std::uint32_t> remainingInDegree;
	remainingInDegree.reserve(m_plan.passes.size());
	for (const CompilePassRecord& passRecord : m_plan.passes)
	{
		remainingInDegree.push_back(passRecord.inDegree);
	}

	std::vector<PassIndex> ready;
	ready.reserve(m_plan.passes.size());
	for (const CompilePassRecord& passRecord : m_plan.passes)
	{
		if (passRecord.alive && passRecord.inDegree == 0)
		{
			ready.push_back(passRecord.index);
		}
	}

	for (std::size_t readyIndex = 0; readyIndex < ready.size(); ++readyIndex)
	{
		const PassIndex passIndex = ready[readyIndex];
		m_plan.executionOrder.push_back(passIndex);

		const CompilePassRecord& passRecord = m_plan.passes[passIndex];
		for (const PassIndex successor : passRecord.successors)
		{
			assert(successor < remainingInDegree.size());
			assert(remainingInDegree[successor] > 0);
			--remainingInDegree[successor];
			if (remainingInDegree[successor] == 0 && m_plan.passes[successor].alive)
			{
				ready.push_back(successor);
			}
		}
	}
}

void FrameGraphCompiler::ValidateExecutionOrder() const noexcept
{
	std::size_t alivePassCount = 0;
	std::vector<bool> visited(m_plan.passes.size(), false);
	std::vector<std::size_t> orderPosition(m_plan.passes.size(), static_cast<std::size_t>(-1));

	for (const CompilePassRecord& passRecord : m_plan.passes)
	{
		if (passRecord.alive)
		{
			++alivePassCount;
		}
		for (const PassIndex successor : passRecord.successors)
		{
			assert(successor < m_plan.passes.size());
		}
	}

	assert(m_plan.executionOrder.size() == alivePassCount);
	for (const PassIndex passIndex : m_plan.executionOrder)
	{
		assert(passIndex < m_plan.passes.size());
		assert(m_plan.passes[passIndex].alive);
		assert(!visited[passIndex]);
		visited[passIndex] = true;
	}

	for (std::size_t orderIndex = 0; orderIndex < m_plan.executionOrder.size(); ++orderIndex)
	{
		orderPosition[m_plan.executionOrder[orderIndex]] = orderIndex;
	}

	for (const CompilePassRecord& passRecord : m_plan.passes)
	{
		if (!passRecord.alive)
		{
			continue;
		}

		for (const PassIndex dependency : passRecord.dependsOn)
		{
			assert(dependency < orderPosition.size());
			assert(orderPosition[dependency] != static_cast<std::size_t>(-1));
			assert(orderPosition[passRecord.index] != static_cast<std::size_t>(-1));
			assert(orderPosition[dependency] < orderPosition[passRecord.index]);
		}
	}
}

void FrameGraphCompiler::BuildPassResourceVersionDependencies(CompilePassRecord& passRecord) noexcept
{
	for (const PassResourceDeclaration& declaration : passRecord.declarations)
	{
		if (!declaration.handle.IsValid())
		{
			continue;
		}

		CompileResourceEntry& resource = GetCompiledResourceEntry(declaration.handle);
		if (IsReadOnlyUsage(declaration.usage))
		{
			RegisterReadDependency(passRecord, resource);
		}
		else if (IsWriteOnlyUsage(declaration.usage))
		{
			RegisterWriteDependency(passRecord, resource);
		}
		else
		{
			assert(false);
		}
	}
}

void FrameGraphCompiler::RegisterReadDependency(CompilePassRecord& passRecord, CompileResourceEntry& resource) noexcept
{
	ResourceVersion& currentVersion = GetCurrentResourceVersion(resource);
	if (currentVersion.writerPass != FrameGraph::INVALID_PASS_INDEX && currentVersion.writerPass != passRecord.index)
	{
		AddRawDependency(passRecord.dependsOn, currentVersion.writerPass);
	}

	RegisterVersionReader(currentVersion, passRecord.index);
}

void FrameGraphCompiler::RegisterWriteDependency(CompilePassRecord& passRecord, CompileResourceEntry& resource) noexcept
{
	const ResourceVersion& currentVersion = GetCurrentResourceVersion(resource);
	if (currentVersion.writerPass != FrameGraph::INVALID_PASS_INDEX && currentVersion.writerPass != passRecord.index)
	{
		AddRawDependency(passRecord.dependsOn, currentVersion.writerPass);
	}

	for (const PassIndex readerPass : currentVersion.readerPasses)
	{
		if (readerPass == passRecord.index)
		{
			continue;
		}

		AddRawDependency(passRecord.dependsOn, readerPass);
	}

	resource.currentVersion = static_cast<std::uint32_t>(resource.versions.size());
	resource.versions.push_back(
	    ResourceVersion{.handle = resource.handle, .version = resource.currentVersion, .writerPass = passRecord.index});
}
