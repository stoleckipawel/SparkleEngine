#include "PCH.h"
#include "FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

class FrameGraphCompilerDependenciesOperations final
{
  public:
	static void AddRawDependency(std::vector<FrameGraphPassIndex>& dependsOn, FrameGraphPassIndex dependency) noexcept
	{
		if (dependency == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			return;
		}

		dependsOn.push_back(dependency);
	}

	static void RegisterVersionReader(FrameGraphResourceVersion& version, FrameGraphPassIndex readerPass) noexcept
	{
		if (readerPass == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			return;
		}

		const auto it = std::find(version.readerPasses.begin(), version.readerPasses.end(), readerPass);
		if (it == version.readerPasses.end())
		{
			version.readerPasses.push_back(readerPass);
		}
	}
};

void FrameGraphCompiler::BuildResourceVersionGraph() noexcept
{
	for (FrameGraphPassNode& passRecord : m_plan.passes)
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
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		std::vector<FrameGraphPassIndex> deduplicated;
		deduplicated.reserve(passRecord.dependsOn.size());

		for (const FrameGraphPassIndex dependency : passRecord.dependsOn)
		{
			if (dependency == INVALID_FRAME_GRAPH_PASS_INDEX || dependency == passRecord.index)
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
	std::vector<FrameGraphPassIndex> rootPasses;
	rootPasses.reserve(m_plan.passes.size());

	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.alive = false;
		if (GetRootPassReason(passRecord) != nullptr)
		{
			rootPasses.push_back(passRecord.index);
		}
	}

	for (const FrameGraphPassIndex rootPass : rootPasses)
	{
		MarkPassAliveRecursive(rootPass);
	}
}

void FrameGraphCompiler::MarkPassAliveRecursive(FrameGraphPassIndex passIndex) noexcept
{
	assert(passIndex < m_plan.passes.size());
	FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
	if (passRecord.alive)
	{
		return;
	}

	passRecord.alive = true;
	for (const FrameGraphPassIndex dependency : passRecord.dependsOn)
	{
		if (dependency == INVALID_FRAME_GRAPH_PASS_INDEX)
		{
			continue;
		}

		MarkPassAliveRecursive(dependency);
	}
}

const char* FrameGraphCompiler::GetRootPassReason(const FrameGraphPassNode& passRecord) const noexcept
{
	if (WritesBackBuffer(passRecord))
	{
		return "backbuffer-output";
	}

	if (WritesProductRoot(passRecord))
	{
		return "product-output";
	}

	return nullptr;
}

bool FrameGraphCompiler::WritesBackBuffer(const FrameGraphPassNode& passRecord) const noexcept
{
	for (const PassResourceDeclaration& declaration : passRecord.declarations)
	{
		if (!declaration.handle.IsValid() || !WritesToUsage(declaration.usage))
		{
			continue;
		}

		const FrameGraphResourceNode& resource = GetCompiledResourceEntry(declaration.handle);
		if (resource.kind == FrameGraphResourceKind::BackBuffer)
		{
			return true;
		}
	}

	return false;
}

bool FrameGraphCompiler::WritesProductRoot(const FrameGraphPassNode& passRecord) const noexcept
{
	for (const FrameGraphProductRoot& productRoot : m_plan.productRoots)
	{
		if (!productRoot.handle.IsValid())
		{
			continue;
		}

		const FrameGraphResourceNode& resource = GetCompiledResourceEntry(productRoot.handle);
		const FrameGraphResourceVersion& finalVersion = GetCurrentResourceVersion(resource);
		if (finalVersion.writerPass == passRecord.index)
		{
			return true;
		}
	}

	return false;
}

void FrameGraphCompiler::BuildPassSuccessorsAndInDegrees() noexcept
{
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.successors.clear();
		passRecord.inDegree = 0;
	}

	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		if (!passRecord.alive)
		{
			continue;
		}

		for (const FrameGraphPassIndex dependency : passRecord.dependsOn)
		{
			assert(dependency < m_plan.passes.size());
			FrameGraphPassNode& dependencyPass = m_plan.passes[dependency];
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
	for (const FrameGraphPassNode& passRecord : m_plan.passes)
	{
		remainingInDegree.push_back(passRecord.inDegree);
	}

	std::vector<FrameGraphPassIndex> ready;
	ready.reserve(m_plan.passes.size());
	for (const FrameGraphPassNode& passRecord : m_plan.passes)
	{
		if (passRecord.alive && passRecord.inDegree == 0)
		{
			ready.push_back(passRecord.index);
		}
	}

	for (std::size_t readyIndex = 0; readyIndex < ready.size(); ++readyIndex)
	{
		const FrameGraphPassIndex passIndex = ready[readyIndex];
		m_plan.executionOrder.push_back(passIndex);

		const FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
		for (const FrameGraphPassIndex successor : passRecord.successors)
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

	for (const FrameGraphPassNode& passRecord : m_plan.passes)
	{
		if (passRecord.alive)
		{
			++alivePassCount;
		}
		for (const FrameGraphPassIndex successor : passRecord.successors)
		{
			assert(successor < m_plan.passes.size());
		}
	}

	assert(m_plan.executionOrder.size() == alivePassCount);
	for (const FrameGraphPassIndex passIndex : m_plan.executionOrder)
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

	for (const FrameGraphPassNode& passRecord : m_plan.passes)
	{
		if (!passRecord.alive)
		{
			continue;
		}

		for (const FrameGraphPassIndex dependency : passRecord.dependsOn)
		{
			assert(dependency < orderPosition.size());
			assert(orderPosition[dependency] != static_cast<std::size_t>(-1));
			assert(orderPosition[passRecord.index] != static_cast<std::size_t>(-1));
			assert(orderPosition[dependency] < orderPosition[passRecord.index]);
		}
	}
}

void FrameGraphCompiler::BuildPassResourceVersionDependencies(FrameGraphPassNode& passRecord) noexcept
{
	for (const PassResourceDeclaration& declaration : passRecord.declarations)
	{
		if (!declaration.handle.IsValid())
		{
			continue;
		}

		FrameGraphResourceNode& resource = GetCompiledResourceEntry(declaration.handle);
		if (IsReadOnlyUsage(declaration.usage))
		{
			RegisterReadDependency(passRecord, resource);
		}
		else if (IsReadWriteUsage(declaration.usage))
		{
			RegisterReadDependency(passRecord, resource);
			RegisterWriteDependency(passRecord, resource);
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

void FrameGraphCompiler::RegisterReadDependency(FrameGraphPassNode& passRecord, FrameGraphResourceNode& resource) noexcept
{
	FrameGraphResourceVersion& currentVersion = GetCurrentResourceVersion(resource);
	if (currentVersion.writerPass != INVALID_FRAME_GRAPH_PASS_INDEX && currentVersion.writerPass != passRecord.index)
	{
		FrameGraphCompilerDependenciesOperations::AddRawDependency(passRecord.dependsOn, currentVersion.writerPass);
	}

	FrameGraphCompilerDependenciesOperations::RegisterVersionReader(currentVersion, passRecord.index);
}

void FrameGraphCompiler::RegisterWriteDependency(FrameGraphPassNode& passRecord, FrameGraphResourceNode& resource) noexcept
{
	const FrameGraphResourceVersion& currentVersion = GetCurrentResourceVersion(resource);
	if (currentVersion.writerPass != INVALID_FRAME_GRAPH_PASS_INDEX && currentVersion.writerPass != passRecord.index)
	{
		FrameGraphCompilerDependenciesOperations::AddRawDependency(passRecord.dependsOn, currentVersion.writerPass);
	}

	for (const FrameGraphPassIndex readerPass : currentVersion.readerPasses)
	{
		if (readerPass == passRecord.index)
		{
			continue;
		}

		FrameGraphCompilerDependenciesOperations::AddRawDependency(passRecord.dependsOn, readerPass);
	}

	resource.currentVersion = static_cast<std::uint32_t>(resource.versions.size());
	resource.versions.push_back(
	    FrameGraphResourceVersion{.handle = resource.handle, .version = resource.currentVersion, .writerPass = passRecord.index});
}
